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

        return Buffer{ size, buffer, memory};
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
        const auto layout = shader_module.descriptor_set_layouts.at(set_index);
        const auto pool = shader_module.descriptor_pools.at(set_index).pool;

        // We want to create N sets of the same descriptor set layout so we duplicate
        // the layout N times.  Vulkan will then create N sets with that layout.
        std::vector<vk::DescriptorSetLayout> layouts(descriptor_set_count, layout);

        const vk::DescriptorSetAllocateInfo allocate_info {
            .descriptorPool = pool,
            .descriptorSetCount = descriptor_set_count,
            .pSetLayouts = layouts.data()
        };
        
        auto descriptor_sets = m_device.allocateDescriptorSets(allocate_info);

        return DescriptorSetChunk{ descriptor_sets, pool };
    }

    void Renderer::destroy_descriptor_sets(DescriptorSetChunk& descriptor_sets) {
        m_device.freeDescriptorSets(descriptor_sets.pool, descriptor_sets.sets);
        descriptor_sets.sets.resize(0);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // SHADERS                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<vk::DescriptorSetLayout> Renderer::create_descriptor_set_layouts(
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
    ) {
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts(descriptor_set_layout_bindings.size());
        for (auto i = 0; i < descriptor_set_layout_bindings.size(); i++) {
            const vk::DescriptorSetLayoutCreateInfo create_info{
                .bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings[i].size()),
                .pBindings = descriptor_set_layout_bindings[i].data()
            };
            descriptor_set_layouts[i] = m_device.createDescriptorSetLayout(create_info);
        }
        return descriptor_set_layouts;
    }

    constexpr auto DEFAULT_MAX_DESCRIPTOR_SETS = 32768;

    std::vector<ShaderModule::DescriptorPool> Renderer::create_descriptor_pools(
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
    ) {
        std::vector<ShaderModule::DescriptorPool> descriptor_pools;

        for (auto set = 0; set < descriptor_set_layout_bindings.size(); set++) {

            /*
            From the Vulkan Spec:
                If multiple VkDescriptorPoolSize structures containing the same descriptor type appear in the 
                pPoolSizes array then the pool will be created with enough storage for the total number of descriptors
                of each type.
            
            Therefore we can simply create a pool size structure for each entry in the binding array and let
            the driver handle coalescing them for us.
            */

            std::vector<vk::DescriptorPoolSize> pool_sizes(descriptor_set_layout_bindings[set].size());
            for (auto i = 0; i < descriptor_set_layout_bindings[set].size(); i++) {
                const auto& binding = descriptor_set_layout_bindings[set][i];
                pool_sizes[i].type = binding.descriptorType;
                pool_sizes[i].descriptorCount = binding.descriptorCount;
            }

            const vk::DescriptorPoolCreateInfo create_info {
                .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets = DEFAULT_MAX_DESCRIPTOR_SETS,
                .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                .pPoolSizes = pool_sizes.data()
            };

            descriptor_pools.push_back(ShaderModule::DescriptorPool{
                .pool = m_device.createDescriptorPool(create_info),
                .allocated_sets = 0,
                .max_sets = DEFAULT_MAX_DESCRIPTOR_SETS,
                .highwater_sets = 0,
            });
        }

        return descriptor_pools;
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
        auto descriptor_set_layouts = create_descriptor_set_layouts(descriptor_set_layout_bindings);
        auto push_constant_ranges = reflection.get_push_constant_ranges();

        return ShaderModule {
            .shader_module = module,
            .descriptor_set_layout_bindings = descriptor_set_layout_bindings,
            .descriptor_set_layouts = descriptor_set_layouts,
            .push_constant_ranges = push_constant_ranges,
            .descriptor_pools = create_descriptor_pools(descriptor_set_layout_bindings)
        };
    }

    void Renderer::destroy_shader_module(ShaderModule& module) {
        for (auto& pool : module.descriptor_pools) {
            assert(pool.allocated_sets == 0);
            m_device.destroyDescriptorPool(pool.pool);
            pool.pool = VK_NULL_HANDLE;
        }

        for (auto& layout : module.descriptor_set_layouts) {
            m_device.destroyDescriptorSetLayout(layout);
            layout = VK_NULL_HANDLE;
        }

        m_device.destroyShaderModule(module.shader_module);
        module.shader_module = VK_NULL_HANDLE;
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
            .bind_point = vk::PipelineBindPoint::eCompute,
            .layout = layout,
            .pipeline = result.value
        };
    }

    void Renderer::destroy_pipeline(Pipeline& pipeline) {
        m_device.destroyPipeline(pipeline.pipeline);
        pipeline.pipeline = VK_NULL_HANDLE;
        m_device.destroyPipelineLayout(pipeline.layout);
        pipeline.layout = VK_NULL_HANDLE;
    }
}