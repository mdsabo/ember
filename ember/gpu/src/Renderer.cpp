#include "Renderer.h"

#include "SPIRV.h"
#include "ShaderReflection.h"
#include "Util.h"

namespace ember::gpu {

    Renderer::Renderer(std::shared_ptr<const GraphicsDevice> device): m_gpu(device) {
        auto [vk_device, queue, command_pool] = device->create_render_objects();
        m_device = vk_device;
        m_queue = queue;
        m_command_pool = command_pool;
    }

    Renderer::~Renderer() {
        m_device.destroyCommandPool(m_command_pool);
        m_device.destroy();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Command Buffers                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::CommandBuffer Renderer::record_render_commands(const CommandRecordFn& fn) {
        const vk::CommandBufferAllocateInfo allocate_info {
            .commandPool = m_command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        auto command_buffer = m_device.allocateCommandBuffers(allocate_info).front();
        command_buffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });

        auto recorder = CommandRecorder(command_buffer);
        fn(recorder);
        return command_buffer;
    }

    void Renderer::submit_command_buffers(const std::vector<const vk::CommandBuffer>& command_buffers) {
        const vk::SubmitInfo submit_info {
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = static_cast<uint32_t>(command_buffers.size()),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        m_queue.submit(submit_info);

        m_device.freeCommandBuffers(m_command_pool, command_buffers);
    }

    void Renderer::submit_command_buffer(const vk::CommandBuffer command_buffer) {
        submit_command_buffers({ command_buffer });
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

        return Buffer(size, buffer, memory);
    }

    void Renderer::destroy_buffer(Buffer& buffer) {
        m_device.freeMemory(buffer.memory);
        buffer.memory = VK_NULL_HANDLE;
        m_device.destroyBuffer(buffer.buffer);
        buffer.buffer = VK_NULL_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Descriptor Sets                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    DescriptorSetChunk Renderer::create_descriptor_sets(
        ShaderModule& shader_module,
        uint32_t set_index,
        uint32_t descriptor_set_count
    ) {
        auto& dsai = shader_module.descriptor_set_allocation_infos.at(set_index);
        const auto layout = dsai.layout;
        const auto pool = dsai.pool;

        // We want to create N sets of the same descriptor set layout so we duplicate
        // the layout N times.  Vulkan will then create N sets with that layout.
        std::vector<vk::DescriptorSetLayout> layouts(descriptor_set_count, layout);

        const vk::DescriptorSetAllocateInfo allocate_info {
            .descriptorPool = pool,
            .descriptorSetCount = descriptor_set_count,
            .pSetLayouts = layouts.data()
        };

        auto descriptor_sets = m_device.allocateDescriptorSets(allocate_info);
        dsai.allocated_sets += descriptor_set_count;
        dsai.highwater_sets = std::max(dsai.allocated_sets, dsai.highwater_sets);

        return DescriptorSetChunk(descriptor_sets, set_index);
    }

    void Renderer::destroy_descriptor_sets(ShaderModule& shader_module, DescriptorSetChunk& descriptor_sets) {
        auto& dsai = shader_module.descriptor_set_allocation_infos.at(descriptor_sets.set_index);
        m_device.freeDescriptorSets(dsai.pool, descriptor_sets.sets);
        dsai.allocated_sets -= descriptor_sets.sets.size();
        descriptor_sets.sets.resize(0);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // SHADERS                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////

    constexpr auto DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL = 32768;

    std::vector<ShaderModule::DescriptorSetAllocationInfo> Renderer::create_descriptor_set_allocation_infos(
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
    ) {
        std::vector<ShaderModule::DescriptorSetAllocationInfo> descriptor_set_allocation_infos(
            descriptor_set_layout_bindings.size()
        );

        for (auto set = 0; set < descriptor_set_layout_bindings.size(); set++) {
            const auto& dslb = descriptor_set_layout_bindings[set];
            auto& dsai = descriptor_set_allocation_infos[set];

            const vk::DescriptorSetLayoutCreateInfo layout_create_info {
                .bindingCount = static_cast<uint32_t>(dslb.size()),
                .pBindings = dslb.data()
            };
            dsai.layout = m_device.createDescriptorSetLayout(layout_create_info);

            /*
            From the Vulkan Spec:
                If multiple VkDescriptorPoolSize structures containing the same descriptor type appear in the
                pPoolSizes array then the pool will be created with enough storage for the total number of descriptors
                of each type.

            Therefore we can simply create a pool size structure for each entry in the binding array and let
            the driver handle coalescing them for us.
            */
            std::vector<vk::DescriptorPoolSize> pool_sizes(dslb.size());
            for (auto i = 0; i < dslb.size(); i++) {
                const auto& binding = dslb[i];
                pool_sizes[i].type = binding.descriptorType;
                pool_sizes[i].descriptorCount = binding.descriptorCount;
            }

            const vk::DescriptorPoolCreateInfo pool_create_info {
                .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL,
                .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                .pPoolSizes = pool_sizes.data()
            };

            dsai.pool = m_device.createDescriptorPool(pool_create_info);
            dsai.allocated_sets = 0;
            dsai.max_sets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL;
            dsai.highwater_sets = 0;
        }
        return descriptor_set_allocation_infos;
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
        auto push_constant_ranges = reflection.get_push_constant_ranges();

        auto descriptor_allocation_infos = create_descriptor_set_allocation_infos(descriptor_set_layout_bindings);

        return ShaderModule(
            module,
            descriptor_set_layout_bindings,
            descriptor_allocation_infos,
            push_constant_ranges
        );
    }

    void Renderer::destroy_shader_module(ShaderModule& module) {
        for (auto& alloc_info : module.descriptor_set_allocation_infos) {
            assert(alloc_info.allocated_sets == 0);

            m_device.destroyDescriptorPool(alloc_info.pool);
            alloc_info.pool = VK_NULL_HANDLE;

            m_device.destroyDescriptorSetLayout(alloc_info.layout);
            alloc_info.layout = VK_NULL_HANDLE;
        }

        m_device.destroyShaderModule(module.shader_module);
        module.shader_module = VK_NULL_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Pipelines                                                                                //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::PipelineLayout Renderer::create_pipeline_layout(const ShaderModule& shader_module) {
        std::vector<vk::DescriptorSetLayout> layouts(shader_module.descriptor_set_allocation_infos.size());
        for (auto i = 0; i < shader_module.descriptor_set_allocation_infos.size(); i++) {
            layouts[i] = shader_module.descriptor_set_allocation_infos[i].layout;
        }

        const vk::PipelineLayoutCreateInfo create_info {
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
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

        return Pipeline(vk::PipelineBindPoint::eCompute, layout, result.value);
    }

    void Renderer::destroy_pipeline(Pipeline& pipeline) {
        m_device.destroyPipeline(pipeline.pipeline);
        pipeline.pipeline = VK_NULL_HANDLE;
        m_device.destroyPipelineLayout(pipeline.layout);
        pipeline.layout = VK_NULL_HANDLE;
    }
}