#include "Renderer.h"

#include "SPIRV.h"
#include "ShaderReflection.h"
#include "Util.h"

namespace ember::gpu {

    Renderer::Renderer(std::shared_ptr<const GraphicsDevice> device): m_gpu(device), m_allocated_command_buffers(0) {
        auto [vk_device, queue, command_pool] = device->create_render_objects();
        m_device = vk_device;
        m_queue = queue;
        m_command_pool = command_pool;
    }

    Renderer::~Renderer() {
        m_device.destroyCommandPool(m_command_pool);
        m_device.destroy();
    }

    vk::SwapchainKHR Renderer::create_swapchain_for_surface(
        vk::SurfaceKHR surface,
        vk::Extent2D window_extent,
        vk::SwapchainKHR old_swapchain
    ) {
        auto create_info = m_gpu->get_swapchain_create_info_for_surface(surface, window_extent, old_swapchain);
        return m_device.createSwapchainKHR(create_info);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Command Buffers                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::CommandBuffer Renderer::allocate_command_buffer(vk::CommandBufferLevel level) {
        const vk::CommandBufferAllocateInfo allocate_info {
            .commandPool = m_command_pool,
            .level = level,
            .commandBufferCount = 1,
        };
        m_allocated_command_buffers++;

        return m_device.allocateCommandBuffers(allocate_info).front();
    }

    vk::CommandBuffer Renderer::create_command_buffer(vk::CommandBufferUsageFlags usage) {
        auto command_buffer = allocate_command_buffer();
        command_buffer.begin({ .flags = usage });
        return command_buffer;
    }

    void Renderer::destroy_command_buffer(vk::CommandBuffer command_buffer) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
        command_buffer = VK_NULL_HANDLE;
    }

    void Renderer::record_command_buffer(vk::CommandBuffer command_buffer, const CommandRecordFn& fn) {
        auto recorder = CommandRecorder(command_buffer, m_gpu->queue_family_index());
        fn(recorder);
    }

    void Renderer::record_submit_command_buffer(const CommandRecordFn& fn) {
        auto command_buffer = create_command_buffer();
        record_command_buffer(command_buffer, fn);
        auto fence = submit_command_buffer(command_buffer);
        wait_for_fences({ fence });
        destroy_command_buffer(command_buffer);
    }

    vk::Fence Renderer::submit_command_buffers(
        const ArrayProxy<vk::CommandBuffer>& command_buffers,
        const ArrayProxy<vk::Semaphore>& wait_semaphores,
        const ArrayProxy<vk::Semaphore>& signal_sempahores
    ) {
        for (auto& cmdbuf : command_buffers) {
            cmdbuf.end();
        }

        const vk::SubmitInfo submit_info {
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = command_buffers.size(),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        auto fence = m_device.createFence({});
        m_queue.submit(submit_info, fence);
        return fence;
    }

    vk::Fence Renderer::submit_command_buffer(
        vk::CommandBuffer command_buffer,
        const ArrayProxy<vk::Semaphore>& wait_semaphores,
        const ArrayProxy<vk::Semaphore>& signal_sempahores
    ) {
        return submit_command_buffers(command_buffer, wait_semaphores, signal_sempahores);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Memory                                                                                   //
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

    vk::DeviceMemory Renderer::allocate_memory(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties) {
        auto memory_type_index = find_memory_type_index(
            m_gpu->memory_properties(),
            properties,
            requirements.memoryTypeBits
        );

        const vk::MemoryAllocateInfo alloc_info{
            .allocationSize = requirements.size,
            .memoryTypeIndex = memory_type_index,
        };
        return m_device.allocateMemory(alloc_info);
    }

    Buffer* Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        const vk::BufferCreateInfo buffer_create_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        auto buffer = m_device.createBuffer(buffer_create_info);
        auto memory_requirements = m_device.getBufferMemoryRequirements(buffer);
        auto memory = allocate_memory(memory_requirements, properties);
        m_device.bindBufferMemory(buffer, memory, 0);

        auto res = m_buffer_allocator.malloc();
        res->size = size;
        res->buffer = buffer;
        res->memory = memory;
        return res;
    }

    void Renderer::destroy_buffer(Buffer* buffer) {
        m_device.freeMemory(buffer->memory);
        m_device.destroyBuffer(buffer->buffer);
        m_buffer_allocator.free(buffer);
    }

    void Renderer::read_buffer(void* dst, const Buffer* buffer, vk::DeviceSize offset, vk::DeviceSize size) const {
        auto src = m_device.mapMemory(buffer->memory, offset, size);
        memcpy(dst, src, size);
        m_device.unmapMemory(buffer->memory);
    }

    void Renderer::write_buffer(Buffer* buffer, const void* src, vk::DeviceSize offset, vk::DeviceSize size) {
        auto dst = m_device.mapMemory(buffer->memory, offset, size);
        memcpy(dst, src, size);
        m_device.unmapMemory(buffer->memory);
    }

    void Renderer::bind_buffers(
        const ShaderModule* shader_module,
        const DescriptorSets* descriptor_sets,
        const DescriptorWrite& descriptor_write,
        const ArrayProxy<Buffer*>& buffers
    ) {
        std::vector<vk::DescriptorBufferInfo> buffer_infos(buffers.size());
        for (auto i = 0; i < buffers.size(); i++) {
            buffer_infos[i].buffer = buffers.data()[i]->buffer;
            buffer_infos[i].offset = 0;
            buffer_infos[i].range = buffers.data()[i]->size;
        }

        const vk::WriteDescriptorSet write_descriptor_set {
            .dstSet = descriptor_sets->descriptor_sets[descriptor_write.set_index],
            .dstBinding = descriptor_write.binding_index,
            .dstArrayElement = descriptor_write.array_index,
            .descriptorCount = buffers.size(),
            .descriptorType = shader_module->descriptor_types[descriptor_write.set_index][descriptor_write.binding_index],
            .pBufferInfo = buffer_infos.data(),
        };

        m_device.updateDescriptorSets(write_descriptor_set, {});
    }

    namespace {
        constexpr vk::ImageSubresourceRange IMAGE_WHOLE_SUBRESOURCE_RANGE(vk::ImageAspectFlags aspect_mask) {
            return vk::ImageSubresourceRange {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = vk::RemainingMipLevels,
                .baseArrayLayer = 0,
                .layerCount = vk::RemainingArrayLayers
            };
        }
    }

    Image* Renderer::create_image(const ImageCreateInfo& image_info) {
        const vk::ImageCreateInfo image_create_info {
            .imageType = image_info.type,
            .format = image_info.format,
            .extent = image_info.extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eLinear,
            .usage = image_info.usage,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        };
        auto image = m_device.createImage(image_create_info);
        auto memory_requirements = m_device.getImageMemoryRequirements(image);
        auto memory = allocate_memory(memory_requirements, image_info.memory_properties);
        m_device.bindImageMemory(image, memory, 0);

        const vk::ImageViewCreateInfo view_create_info {
            .image = image,
            .viewType = vk::ImageViewType::e2D, // FIXME
            .format = image_info.format,
            .components = vk::ComponentMapping {
                .r = vk::ComponentSwizzle::eR,
                .g = vk::ComponentSwizzle::eG,
                .b = vk::ComponentSwizzle::eB,
                .a = vk::ComponentSwizzle::eA,
            },
            .subresourceRange = IMAGE_WHOLE_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor),
        };
        auto view = m_device.createImageView(view_create_info);

        auto res = m_image_allocator.malloc();
        res->extent = image_info.extent;
        res->layout = image_info.layout;
        res->image = image;
        res->view = view;
        res->memory = memory;

        record_submit_command_buffer([&res, &image_info](CommandRecorder& recorder) {
            recorder.transition_image_layout(res, image_info.layout);
        });

        return res;
    }

    void Renderer::destroy_image(Image* image) {
        m_device.destroyImageView(image->view);
        m_device.freeMemory(image->memory);
        m_device.destroyImage(image->image);
        m_image_allocator.free(image);
    }

    void Renderer::read_image(void* dst, const Image* image) {
        auto properties = m_device.getImageMemoryRequirements(image->image);
        auto src = m_device.mapMemory(image->memory, 0, properties.size);
        memcpy(dst, src, properties.size);
        m_device.unmapMemory(image->memory);
    }

    void Renderer::bind_images(
        const ShaderModule* shader_module,
        const DescriptorSets* descriptor_sets,
        const DescriptorWrite& descriptor_write,
        const ArrayProxy<Image*>& images
    ) {
        std::vector<vk::DescriptorImageInfo> image_infos(images.size());
        for (auto i = 0; i < images.size(); i++) {
            image_infos[i].imageView = images.data()[i]->view;
            image_infos[i].imageLayout = vk::ImageLayout::eGeneral;
        }

        const vk::WriteDescriptorSet write_descriptor_set {
            .dstSet = descriptor_sets->descriptor_sets[descriptor_write.set_index],
            .dstBinding = descriptor_write.binding_index,
            .dstArrayElement = descriptor_write.array_index,
            .descriptorCount = images.size(),
            .descriptorType = shader_module->descriptor_types[descriptor_write.set_index][descriptor_write.binding_index],
            .pImageInfo = image_infos.data(),
        };

        m_device.updateDescriptorSets(write_descriptor_set, {});
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Descriptor Sets                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    DescriptorSets* Renderer::create_descriptor_sets(
        ShaderModule* shader_module,
        uint32_t set_index,
        uint32_t descriptor_set_count
    ) {
        auto& descriptor_pool = shader_module->descriptor_pools[set_index];
        const auto layout = shader_module->descriptor_set_layouts[set_index];

        // We want to create N sets of the same descriptor set layout so we duplicate
        // the layout N times.  Vulkan will then create N sets with that layout.
        std::vector<vk::DescriptorSetLayout> layouts(descriptor_set_count, layout);

        const vk::DescriptorSetAllocateInfo allocate_info {
            .descriptorPool = descriptor_pool.pool,
            .descriptorSetCount = descriptor_set_count,
            .pSetLayouts = layouts.data()
        };

        auto descriptor_sets = m_descriptor_sets_allocator.malloc();
        descriptor_sets->shader_module = shader_module;
        descriptor_sets->set_index = set_index;
        descriptor_sets->descriptor_sets = m_device.allocateDescriptorSets(allocate_info);

        descriptor_pool.allocated_sets += descriptor_set_count;
        descriptor_pool.highwater_sets = std::max(descriptor_pool.allocated_sets, descriptor_pool.highwater_sets);

        return descriptor_sets;
    }

    void Renderer::destroy_descriptor_sets(DescriptorSets* descriptor_sets) {
        auto& pool = descriptor_sets->shader_module->descriptor_pools[descriptor_sets->set_index];
        m_device.freeDescriptorSets(pool.pool, descriptor_sets->descriptor_sets);
        pool.allocated_sets -= descriptor_sets->descriptor_sets.size();
        m_descriptor_sets_allocator.free(descriptor_sets);
    }

    void Renderer::merge_descriptor_sets(DescriptorSets* dst, DescriptorSets* src) {
        assert(src->shader_module == dst->shader_module);
        assert(src->set_index == dst->set_index);

        dst->descriptor_sets.reserve(dst->descriptor_sets.size() + src->descriptor_sets.size());
        std::copy(
            src->descriptor_sets.begin(),
            src->descriptor_sets.end(),
            std::back_inserter(dst->descriptor_sets)
        );
        m_descriptor_sets_allocator.free(src);
    }




    namespace {
        void merge_shader_descriptor_set_layout_bindings(
            const ShaderModule2* shader,
            DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>>& layout_bindings
        ) {
            auto shader_bindings = shader->reflection.get_descriptor_set_bindings();

            // For each set in the shader's set layouts
            for (auto set = 0; set < shader_bindings.size(); set++) {
                // For each binding in that set
                for (const auto& shader_binding : shader_bindings[set]) {
                    // See if a matching binding index is already in our layout bindings
                    auto iter = std::find_if(
                        layout_bindings[set].begin(),
                        layout_bindings[set].end(),
                        [shader_binding](const vk::DescriptorSetLayoutBinding& b) {
                            return shader_binding.binding == b.binding;
                        }
                    );

                    if (iter != layout_bindings[set].end()) {
                        // If there is, then make sure the shaders agree on the layout
                        assert(iter->descriptorType == shader_binding.descriptorType);
                        assert(iter->descriptorCount == shader_binding.descriptorCount);
                        // Then add the new stage to the stage flags
                        iter->stageFlags |= shader_binding.stageFlags;
                    } else {
                        // Otherwise add this new binding to our layout bindings
                        layout_bindings[set].push_back(shader_binding);
                    }
                }
            }
        }

        DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>> get_descriptor_set_layout_bindings(
            const ArrayProxy<ShaderModule2*>& shader_stages
        ) {
            DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>> bindings;
            for (const auto shader : shader_stages) {
                merge_shader_descriptor_set_layout_bindings(shader, bindings);
            }
            return bindings;
        }
    }

    DescriptorSetBlueprint* Renderer::create_descriptor_set_blueprint(
        const ArrayProxy<ShaderModule2*>& shader_stages
    ) {
        auto descriptor_set_allocator = m_descriptor_set_blueprint_allocator.malloc();

        descriptor_set_allocator->layout_bindings = get_descriptor_set_layout_bindings(shader_stages);

        const auto& layout_bindings = descriptor_set_allocator->layout_bindings;
        for (auto set = 0; set < MAX_DESCRIPTOR_SETS; set++) {
            const auto& set_bindings = layout_bindings[set];
            if (set_bindings.size() == 0) continue; // skip empty sets

            const vk::DescriptorSetLayoutCreateInfo layout_create_info {
                .bindingCount = static_cast<uint32_t>(set_bindings.size()),
                .pBindings = set_bindings.data()
            };
            descriptor_set_allocator->layouts[set] = m_device.createDescriptorSetLayout(layout_create_info);

            /*
            From the Vulkan Spec:
                If multiple VkDescriptorPoolSize structures containing the same descriptor type appear in the
                pPoolSizes array then the pool will be created with enough storage for the total number of descriptors
                of each type.

            Therefore we can simply create a pool size structure for each entry in the binding array and let
            the driver handle coalescing them for us.
            */
            std::vector<vk::DescriptorPoolSize> pool_sizes;
            pool_sizes.reserve(set_bindings.size());
            for (auto i = 0; i < set_bindings.size(); i++) {
                pool_sizes.push_back({
                    .type = set_bindings[i].descriptorType,
                    .descriptorCount = set_bindings[i].descriptorCount
                });
            }

            const vk::DescriptorPoolCreateInfo pool_create_info {
                .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL,
                .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                .pPoolSizes = pool_sizes.data()
            };
            descriptor_set_allocator->pools[set].pool = m_device.createDescriptorPool(pool_create_info);
        }

        return descriptor_set_allocator;
    }

    void Renderer::destroy_descriptor_set_blueprint(DescriptorSetBlueprint* dsa) {
        for (auto layout : dsa->layouts) {
            if (layout != VK_NULL_HANDLE) m_device.destroyDescriptorSetLayout(layout);
        }
        for (auto& pool : dsa->pools) {
            if (pool.pool != VK_NULL_HANDLE) m_device.destroyDescriptorPool(pool.pool);
        }
        m_descriptor_set_blueprint_allocator.free(dsa);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Shaders                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////

    ShaderModule* Renderer::create_shader_module(const std::filesystem::path& path) {
        auto spirv = compile_glsl_to_spirv(path, false);
        const vk::ShaderModuleCreateInfo create_info {
            .codeSize = spirv_code_size(spirv), // codeSize is the size, in bytes (not words for some reason), of the code pointed to by pCode.
            .pCode = spirv.data()
        };

        auto reflection = ShaderReflection(spirv);

        auto shader_module = m_shader_module_allocator.malloc();
        shader_module->stage = reflection.get_shader_stage();
        shader_module->shader_module = m_device.createShaderModule(create_info);

        auto descriptor_set_layout_bindings = reflection.get_descriptor_set_bindings();
        shader_module->descriptor_set_count = descriptor_set_layout_bindings.size();
        for (auto set = 0; set < descriptor_set_layout_bindings.size(); set++) {
            shader_module->descriptor_binding_counts[set] = descriptor_set_layout_bindings[set].size();

            const vk::DescriptorSetLayoutCreateInfo layout_create_info {
                .bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings[set].size()),
                .pBindings = descriptor_set_layout_bindings[set].data()
            };
            shader_module->descriptor_set_layouts[set] = m_device.createDescriptorSetLayout(layout_create_info);

            /*
            From the Vulkan Spec:
                If multiple VkDescriptorPoolSize structures containing the same descriptor type appear in the
                pPoolSizes array then the pool will be created with enough storage for the total number of descriptors
                of each type.

            Therefore we can simply create a pool size structure for each entry in the binding array and let
            the driver handle coalescing them for us.
            */
            std::array<vk::DescriptorPoolSize, MAX_DESCRIPTOR_BINDINGS_PER_SET> pool_sizes;
            for (auto binding = 0; binding < descriptor_set_layout_bindings[set].size(); binding++) {
                pool_sizes[set].type = descriptor_set_layout_bindings[set][binding].descriptorType;
                pool_sizes[set].descriptorCount = descriptor_set_layout_bindings[set][binding].descriptorCount;

                shader_module->descriptor_types[set][binding] = descriptor_set_layout_bindings[set][binding].descriptorType;
            }

            const vk::DescriptorPoolCreateInfo pool_create_info {
                .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL,
                .poolSizeCount = static_cast<uint32_t>(descriptor_set_layout_bindings[set].size()),
                .pPoolSizes = pool_sizes.data()
            };

            shader_module->descriptor_pools[set].pool = m_device.createDescriptorPool(pool_create_info);
        }

        shader_module->push_constant_ranges = reflection.get_push_constant_ranges();

        return shader_module;
    }

    void Renderer::destroy_shader_module(ShaderModule* module) {
        for (auto i = 0; i < module->descriptor_set_count; i++) {
            const auto& pool = module->descriptor_pools[i];
            assert(pool.allocated_sets == 0);
            m_device.destroyDescriptorPool(pool.pool);
            m_device.destroyDescriptorSetLayout(module->descriptor_set_layouts[i]);
        }

        m_device.destroyShaderModule(module->shader_module);
        m_shader_module_allocator.free(module);
    }




    ShaderModule2* Renderer::create_shader_module2(
        const std::string& glsl,
        const std::string& filename,
        shaderc_shader_kind shader_kind,
        bool optimize
    ) {
        auto spirv = compile_glsl_to_spirv(glsl, filename, shader_kind, optimize);
        const vk::ShaderModuleCreateInfo create_info {
            .codeSize = spirv_code_size(spirv),
            .pCode = spirv.data(),
        };

        auto shader_module = m_shader_module2_allocator.malloc();
        shader_module->module = m_device.createShaderModule(create_info);
        shader_module->reflection = ShaderReflection(spirv);
        return shader_module;
    }

    ShaderModule2* Renderer::create_shader_module2(const std::filesystem::path& path, bool optimize) {
        auto spirv = compile_glsl_to_spirv(path, optimize);
        const vk::ShaderModuleCreateInfo create_info {
            .codeSize = spirv_code_size(spirv),
            .pCode = spirv.data(),
        };

        auto shader_module = m_shader_module2_allocator.malloc();
        shader_module->module = m_device.createShaderModule(create_info);
        shader_module->reflection = ShaderReflection(spirv);
        return shader_module;
    }

    void Renderer::destroy_shader_module2(ShaderModule2* shader_module) {
        m_device.destroyShaderModule(shader_module->module);
        m_shader_module2_allocator.free(shader_module); // cleans up reflection
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Pipelines                                                                                //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::PipelineLayout Renderer::create_pipeline_layout(const ShaderModule* shader_module) {
        const vk::PipelineLayoutCreateInfo create_info {
            .setLayoutCount = shader_module->descriptor_set_count,
            .pSetLayouts = shader_module->descriptor_set_layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(shader_module->push_constant_ranges.size()),
            .pPushConstantRanges = shader_module->push_constant_ranges.data(),
        };
        return m_device.createPipelineLayout(create_info);
    }

    vk::PipelineLayout Renderer::create_pipeline_layout2(
        const ArrayProxy<ShaderModule2*> shader_modules,
        const DescriptorSetBlueprint* descriptor_sets
    ) {
        std::vector<vk::PushConstantRange> push_constant_ranges;
        for (const auto& shader : shader_modules) {
            auto ranges = shader->reflection.get_push_constant_ranges();
            std::copy(ranges.begin(), ranges.end(), std::back_inserter(push_constant_ranges));
        }

        const vk::PipelineLayoutCreateInfo create_info {
            // Spec allows VK_NULL_HANDLES in the layout to indicate unused sets
            .setLayoutCount = static_cast<uint32_t>(descriptor_sets->layouts.size()),
            .pSetLayouts = descriptor_sets->layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data(),
        };
        return m_device.createPipelineLayout(create_info);
    }

    Pipeline* Renderer::create_compute_pipeline(
        const ShaderModule* shader_module,
        const std::string_view& entry_point
    ) {
        auto pipeline = m_pipeline_allocator.malloc();

        pipeline->bind_point = vk::PipelineBindPoint::eCompute;
        pipeline->layout = create_pipeline_layout(shader_module);

        const vk::ComputePipelineCreateInfo create_info {
            .stage = {
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = shader_module->shader_module,
                .pName = entry_point.data()
            },
            .layout = pipeline->layout
        };

        auto result = m_device.createComputePipeline(VK_NULL_HANDLE, create_info);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        pipeline->pipeline = result.value;

        return pipeline;
    }

    Pipeline* Renderer::create_compute_pipeline2(
        ShaderModule2* shader,
        DescriptorSetBlueprint* descriptor_set_blueprint,
        const char* entry_point,
        vk::SpecializationInfo* specialization_info
    ) {
        assert(shader != nullptr);
        assert(descriptor_set_blueprint != nullptr);
        assert(entry_point != nullptr);

        auto pipeline = m_pipeline_allocator.malloc();
        pipeline->bind_point = vk::PipelineBindPoint::eCompute;

        pipeline->layout = create_pipeline_layout2(
            shader,
            descriptor_set_blueprint
        );

        const vk::ComputePipelineCreateInfo create_info {
            .stage = vk::PipelineShaderStageCreateInfo{
                .stage = shader->reflection.get_shader_stage(),
                .module = shader->module,
                .pName = entry_point,
                .pSpecializationInfo = specialization_info
            },
            .layout = pipeline->layout,
        };

        auto result = m_device.createComputePipeline(VK_NULL_HANDLE, create_info);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }
        pipeline->pipeline = result.value;

        return pipeline;
    }

    Pipeline* Renderer::create_graphics_pipeline(
        const ArrayProxy<ShaderModule*>& stages,
        const ArrayProxy<std::string_view>& entry_points,
        const vk::PipelineVertexInputStateCreateInfo& vertex_input_state,
        const vk::PipelineInputAssemblyStateCreateInfo& input_assembly_state,
        const vk::PipelineTessellationStateCreateInfo& tesselation_state,
        const vk::PipelineViewportStateCreateInfo& viewport_state,
        const vk::PipelineRasterizationStateCreateInfo& rasterization_state,
        const vk::PipelineMultisampleStateCreateInfo& multisample_state,
        const vk::PipelineDepthStencilStateCreateInfo& depth_stencil_state,
        const vk::PipelineColorBlendStateCreateInfo& color_blend_state,
        const vk::PipelineDynamicStateCreateInfo& dynamic_state,
        const vk::PipelineRenderingCreateInfo& rendering_info
    ) {
        auto pipeline = m_pipeline_allocator.malloc();

        constexpr auto MAX_SHADER_STAGES = 6;
        assert(stages.size() <= MAX_SHADER_STAGES);
        std::array<vk::PipelineShaderStageCreateInfo, 6> shader_stages;
        for (auto i = 0; i < stages.size(); i++) {
            shader_stages[i] = vk::PipelineShaderStageCreateInfo{
                .stage = stages.data()[i]->stage,
                .module = stages.data()[i]->shader_module,
                .pName = entry_points.data()[i].data()
            };
        }

        const vk::GraphicsPipelineCreateInfo create_info {
            .pNext = &rendering_info,
            .stageCount = stages.size(),
            .pStages = shader_stages.data(),
            .pVertexInputState = &vertex_input_state,
            .pInputAssemblyState = &input_assembly_state,
            .pTessellationState = &tesselation_state,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterization_state,
            .pDepthStencilState = &depth_stencil_state,
            .pColorBlendState = &color_blend_state,
            .pDynamicState = &dynamic_state
        };

        return pipeline;
    }

    void Renderer::destroy_pipeline(Pipeline* pipeline) {
        m_device.destroyPipeline(pipeline->pipeline);
        m_device.destroyPipelineLayout(pipeline->layout);
        m_pipeline_allocator.free(pipeline);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Synchronization                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    bool Renderer::wait_for_fences(const ArrayProxy<vk::Fence>& fences, std::chrono::nanoseconds timeout) {
        auto result = m_device.waitForFences(
            fences.size(),
            fences.data(),
            VK_TRUE,
            timeout.count()
        );

        for (auto& fence : fences) {
            m_device.destroyFence(fence);
            //fence = VK_NULL_HANDLE;
        }

        // FIXME: Should provide better handling of results in general
        return (result == vk::Result::eSuccess);
    }


    vk::Semaphore Renderer::create_semaphore() {
        return m_device.createSemaphore({});
    }

    void Renderer::destroy_semaphore(vk::Semaphore semaphore) {
        m_device.destroySemaphore(semaphore);
    }
}