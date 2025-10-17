#include "Renderer.h"

#include "SPIRV.h"
#include "ShaderReflection.h"
#include "Util.h"

namespace ember::gpu {

    namespace {

        #define DESCRIPTOR_POOL_SIZE(ty, dcount) \
            vk::DescriptorPoolSize{ .type = ty, .descriptorCount = dcount }

        constexpr std::array<vk::DescriptorPoolSize, 11> make_descriptor_pool_sizes(const DescriptorSetCounts& counts) {
            return std::array{
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eSampler, counts.samplers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eCombinedImageSampler, counts.combined_image_samplers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eSampledImage, counts.sampled_images),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eStorageImage, counts.storage_images),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eUniformTexelBuffer, counts.uniform_texel_buffers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eStorageTexelBuffer, counts.storage_texel_buffers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eUniformBuffer, counts.uniform_buffers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eStorageBuffer, counts.storage_buffers),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eUniformBufferDynamic, counts.uniform_buffer_dynamics),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eStorageBufferDynamic, counts.storage_buffer_dynamics),
                DESCRIPTOR_POOL_SIZE(vk::DescriptorType::eInputAttachment, counts.input_attachments),
            };
        }

        constexpr uint32_t max_descriptor_pool_sets(const DescriptorSetCounts& counts) {
            return counts.samplers
            + counts.combined_image_samplers
            + counts.sampled_images
            + counts.storage_images
            + counts.uniform_texel_buffers
            + counts.storage_texel_buffers
            + counts.uniform_buffers
            + counts.storage_buffers
            + counts.uniform_buffer_dynamics
            + counts.storage_buffer_dynamics
            + counts.input_attachments;
        }

        vk::DescriptorPool create_descriptor_pool(vk::Device device, const DescriptorSetCounts& set_counts) {
            auto pool_sizes = make_descriptor_pool_sizes(set_counts);

            const vk::DescriptorPoolCreateInfo create_info{
                .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets = max_descriptor_pool_sets(set_counts),
                .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                .pPoolSizes = pool_sizes.data()
            };
            return device.createDescriptorPool(create_info);
        }
    } // namespace

    Renderer::Renderer(std::shared_ptr<const GraphicsDevice> device): m_gpu(device) {
        auto [vk_device, queue] = device->create_device_and_queue();
        m_device = vk_device;
        m_queue = queue;

        const auto max_descriptor_sets = DescriptorSetCounts{};
        const auto descriptor_pool = create_descriptor_pool(m_device, max_descriptor_sets);
        m_descriptor_pool = DescriptorPool(descriptor_pool, max_descriptor_sets);
    }

    Renderer::~Renderer() {
        m_device.destroy();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // MEMORY                                                                                   //
    //////////////////////////////////////////////////////////////////////////////////////////////

    namespace {
        uint32_t find_memory_type_index(
            vk::PhysicalDeviceMemoryProperties device_memory_properties,
            vk::MemoryPropertyFlags flags,
            uint32_t type_filter
        ) {
            for (auto i = 0; i < device_memory_properties.memoryTypeCount; i++) {
                auto has_type = bool((1ULL << i) & type_filter);
                auto has_properties = vkFlagContains(device_memory_properties.memoryTypes[i].propertyFlags, flags);
                if (has_type && has_properties) return i;
            }
            throw std::runtime_error("No suitable memory types found!");
        }
    } // namespace

    vk::DeviceMemory Renderer::allocate_memory(vk::DeviceSize size, vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties) {
        auto memory_type_index = find_memory_type_index(
            m_gpu->memory_properties(),
            properties,
            requirements.memoryTypeBits
        );

        const vk::MemoryAllocateInfo alloc_info{
            .allocationSize = size,
            .memoryTypeIndex = memory_type_index
        };
        return m_device.allocateMemory(alloc_info);
    }

    Buffer Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        const vk::BufferCreateInfo buffer_create_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        auto buffer = m_device.createBuffer(buffer_create_info);
        auto memory_requirements = m_device.getBufferMemoryRequirements(buffer);
        auto memory = allocate_memory(size, memory_requirements, properties);
        m_device.bindBufferMemory(buffer, memory, 0);

        return Buffer{ size, buffer, memory};
    }

    void Renderer::destroy_buffer(Buffer& buffer) {
        m_device.freeMemory(buffer.memory);
        m_device.destroyBuffer(buffer.buffer);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Descriptor Sets                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////
    // SHADERS                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////

    namespace {
        std::vector<vk::DescriptorSetLayout> make_descriptor_set_layouts(
            vk::Device device,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
        ) {
            std::vector<vk::DescriptorSetLayout> descriptor_set_layouts(descriptor_set_layout_bindings.size());
            for (auto i = 0; i < descriptor_set_layout_bindings.size(); i++) {
                const vk::DescriptorSetLayoutCreateInfo create_info{
                    .bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings[i].size()),
                    .pBindings = descriptor_set_layout_bindings[i].data()
                };
                descriptor_set_layouts[i] = device.createDescriptorSetLayout(create_info);
            }
            return descriptor_set_layouts;
        }
    }

    ShaderModule Renderer::create_shader_module(const std::filesystem::path& path) {
        auto spirv = compile_glsl_to_spirv(path);
        const vk::ShaderModuleCreateInfo create_info {
            .codeSize = spirv_code_size(spirv), // codeSize is the size, in bytes (not words for some reason), of the code pointed to by pCode.
            .pCode = spirv.data()
        };
        auto module = m_device.createShaderModule(create_info);

        auto reflection = ShaderReflection(spirv);
        auto descriptor_set_layout_bindings = reflection.get_descriptor_set_bindings();
        auto descriptor_set_layouts = make_descriptor_set_layouts(m_device, descriptor_set_layout_bindings);
        auto push_constant_ranges = reflection.get_push_constant_ranges();

        return ShaderModule{module, descriptor_set_layout_bindings, descriptor_set_layouts, push_constant_ranges};
    }

    void Renderer::destroy_shader_module(ShaderModule& module) {
        for (auto& layout : module.descriptor_set_layouts) {
            m_device.destroyDescriptorSetLayout(layout);
        }
        m_device.destroyShaderModule(module.shader_module);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Pipelines                                                                                //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::PipelineLayout Renderer::create_pipeline_layout(const ShaderModule& shader_module) {
        const vk::PipelineLayoutCreateInfo create_info {
            .setLayoutCount = static_cast<uint32_t>(shader_module.descriptor_set_layouts.size()),
            .pSetLayouts = shader_module.descriptor_set_layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(shader_module.push_constant_ranges.size()),
            .pPushConstantRanges = shader_module.push_constant_ranges.data(),
        };
        return m_device.createPipelineLayout(create_info);
    }

    Pipeline Renderer::create_compute_pipeline(const ShaderModule& shader_module, const std::string_view& entry_point) {
        auto layout = create_pipeline_layout(shader_module);

        const vk::ComputePipelineCreateInfo create_info {
            .stage = {
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = shader_module.shader_module,
                .pName = entry_point.data()
            },
            .layout = layout
        };

        auto result = m_device.createComputePipeline(VK_NULL_HANDLE, create_info);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        return Pipeline{
            .type = PipelineType::Compute,
            .layout = layout,
            .pipeline = result.value
        };
    }

    void Renderer::destroy_pipeline(Pipeline& pipeline) {
        m_device.destroyPipeline(pipeline.pipeline);
        m_device.destroyPipelineLayout(pipeline.layout);
    }
}