#include "Renderer.h"

#include "ember/util/Log.h"
#include "SPIRV.h"
#include "ShaderReflection.h"
#include "Util.h"
#include "VulkanHelpers.h"

namespace ember::gpu {

    Renderer::Renderer(std::shared_ptr<const GraphicsDevice> device): m_gpu(device), m_allocated_command_buffers(0) {
        auto [vk_device, queue, command_pool] = device->create_render_objects();
        m_device = vk_device;
        m_queue = queue;
        m_command_pool = command_pool;
    }

    Renderer::~Renderer() {
        assert(m_buffer_allocator.allocated_count() == 0);
        assert(m_image_allocator.allocated_count() == 0);
        assert(m_pipeline_allocator.allocated_count() == 0);
        assert(m_shader_module2_allocator.allocated_count() == 0);
        assert(m_descriptor_set_blueprint_allocator.allocated_count() == 0);

        m_device.destroyCommandPool(m_command_pool);
        m_device.destroy();
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

    void Renderer::restart_command_buffer(
        vk::CommandBuffer command_buffer,
        vk::CommandBufferUsageFlags usage
    ) {
        command_buffer.reset();
        command_buffer.begin({ .flags = usage });
    }

    void Renderer::record_command_buffer(vk::CommandBuffer command_buffer, const CommandRecordFn& fn) {
        auto recorder = CommandRecorder(command_buffer, m_gpu->queue_family_index());
        fn(recorder);
    }

    void Renderer::record_submit_command_buffer(const CommandRecordFn& fn, vk::Fence fence) {
        auto command_buffer = create_command_buffer();
        record_command_buffer(command_buffer, fn);

        auto tmp_fence = submit_command_buffer(command_buffer, fence);
        wait_for_fences(std::array{tmp_fence});

        if (fence == VK_NULL_HANDLE) destroy_fence(tmp_fence);
        destroy_command_buffer(command_buffer);
    }

    vk::Fence Renderer::submit_command_buffers(
        const std::span<const vk::CommandBuffer>& command_buffers,
        vk::Fence fence,
        const std::span<const vk::Semaphore>& wait_semaphores,
        const std::span<const vk::PipelineStageFlags>& dst_stage_mask,
        const std::span<const vk::Semaphore>& signal_sempahores
    ) {
        for (auto& cmdbuf : command_buffers) {
            cmdbuf.end();
        }

        const vk::SubmitInfo submit_info {
            .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
            .pWaitSemaphores = wait_semaphores.data(),
            .pWaitDstStageMask = dst_stage_mask.data(),
            .commandBufferCount = static_cast<uint32_t>(command_buffers.size()),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signal_sempahores.size()),
            .pSignalSemaphores = signal_sempahores.data()
        };

        if (fence == VK_NULL_HANDLE) fence = create_fence();

        m_queue.submit(submit_info, fence);
        return fence;
    }

    vk::Fence Renderer::submit_command_buffer(
        vk::CommandBuffer command_buffer,
        vk::Fence fence,
        const std::span<const vk::Semaphore>& wait_semaphores,
        const std::span<const vk::PipelineStageFlags>& dst_stage_mask,
        const std::span<const vk::Semaphore>& signal_sempahores
    ) {
        return submit_command_buffers(
            std::array{command_buffer},
            fence,
            wait_semaphores,
            dst_stage_mask,
            signal_sempahores
        );
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
        std::memcpy(dst, src, size);
        m_device.unmapMemory(buffer->memory);
    }

    void Renderer::write_buffer(Buffer* buffer, const void* src, vk::DeviceSize offset, vk::DeviceSize size) {
        auto memsize = (size == vk::WholeSize) ? (buffer->size) : (size);

        auto dst = m_device.mapMemory(buffer->memory, offset, memsize);
        std::memcpy(dst, src, memsize);
        m_device.unmapMemory(buffer->memory);
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

        const auto view_range = MAX_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor);
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
            .subresourceRange = view_range,
        };
        auto view = m_device.createImageView(view_create_info);

        auto res = m_image_allocator.malloc();
        res->extent = image_info.extent;
        res->layout = vk::ImageLayout::eUndefined;
        res->image = image;
        res->view = view;
        res->memory = memory;

        record_submit_command_buffer([&res, &image_info, &view_range](CommandRecorder& recorder) {
            const CommandRecorder::ImageTransitionInfo info {
                .new_layout = vk::ImageLayout::eGeneral,
                .dst_pipeline_stage = vk::PipelineStageFlagBits::eTransfer,
                .dst_access_mask = vk::AccessFlagBits::eTransferRead,
                .subresource_range = view_range
            };
            recorder.transition_image_layout(res, info);
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
        std::memcpy(dst, src, properties.size);
        m_device.unmapMemory(image->memory);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Descriptor Sets                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    namespace {
        void merge_shader_descriptor_set_layout_bindings(
            const ShaderModule* shader,
            DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>>& layout_bindings
        ) {
            auto shader_bindings = shader->reflection->get_descriptor_set_bindings();

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
            const std::span<const ShaderModule* const>& shader_stages
        ) {
            DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>> bindings;
            for (const auto shader : shader_stages) {
                merge_shader_descriptor_set_layout_bindings(shader, bindings);
            }
            return bindings;
        }
    }

    DescriptorSetArray<DescriptorSetBlueprint*> Renderer::create_descriptor_set_blueprints(
        const std::span<const ShaderModule* const>& shader_stages
    ) {
        auto layout_bindings = get_descriptor_set_layout_bindings(shader_stages);

        DescriptorSetArray<DescriptorSetBlueprint*> blueprints;

        for (auto set = 0; set < MAX_DESCRIPTOR_SETS; set++) {
            auto& blueprint = blueprints[set];
            blueprint = m_descriptor_set_blueprint_allocator.malloc();

            if (layout_bindings[set].size() == 0) continue; // skip empty sets

            blueprint->layout_bindings = std::move(layout_bindings[set]);
            const auto& set_bindings = blueprint->layout_bindings;

            const vk::DescriptorSetLayoutCreateInfo layout_create_info {
                .bindingCount = static_cast<uint32_t>(set_bindings.size()),
                .pBindings = set_bindings.data()
            };
            blueprint->layout = m_device.createDescriptorSetLayout(layout_create_info);

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
            blueprint->pool = m_device.createDescriptorPool(pool_create_info);
        }

        return blueprints;
    }

    void Renderer::destroy_descriptor_set_blueprints(
        const std::span<DescriptorSetBlueprint*>& blueprints
    ) {
        for (const auto& blueprint : blueprints) {
            assert(blueprint->allocated_sets == 0);
            m_device.destroyDescriptorPool(blueprint->pool);
            m_device.destroyDescriptorSetLayout(blueprint->layout);
            m_descriptor_set_blueprint_allocator.free(blueprint);
        }
    }

    std::vector<vk::DescriptorSet> Renderer::create_descriptor_sets(
        DescriptorSetBlueprint* descriptor_set_blueprint,
        uint32_t num_descriptor_sets
    ) {
        descriptor_set_blueprint->allocated_sets += num_descriptor_sets;
        descriptor_set_blueprint->highwater_sets = std::max(
            descriptor_set_blueprint->allocated_sets,
            descriptor_set_blueprint->highwater_sets
        );

        // We want to create N sets of the same descriptor set layout so we duplicate
        // the layout N times.  Vulkan will then create N sets with that layout.
        std::vector<vk::DescriptorSetLayout> layouts(num_descriptor_sets, descriptor_set_blueprint->layout);

        const vk::DescriptorSetAllocateInfo allocate_info {
            .descriptorPool = descriptor_set_blueprint->pool,
            .descriptorSetCount = num_descriptor_sets,
            .pSetLayouts = layouts.data()
        };
        return m_device.allocateDescriptorSets(allocate_info);
    }

    void Renderer::destroy_descriptor_sets(
        DescriptorSetBlueprint* descriptor_set_blueprint,
        const std::span<const vk::DescriptorSet>& descriptor_sets
    ) {
        descriptor_set_blueprint->allocated_sets -= descriptor_sets.size();
        m_device.freeDescriptorSets(descriptor_set_blueprint->pool, descriptor_sets);
    }

    namespace {
        vk::WriteDescriptorSet make_descriptor_update(
            const DescriptorSetBlueprint* descriptor_set_blueprint,
            const std::span<const vk::DescriptorSet>& descriptor_sets,
            uint32_t binding_index,
            uint32_t array_index
        ) {
            auto descriptor_type = descriptor_set_blueprint->descriptor_type(binding_index);
            return vk::WriteDescriptorSet {
                // We're basically telling vulkan to bind our buffer/image/view to the descriptor sets
                // in the descriptor_set array.
                // binding_index determines which binding slot those resources will be written into.
                // e.g. layout(binding = 2) buffer ... --> binding_index = 2
                .dstSet = *descriptor_sets.data(),
                .dstBinding = binding_index,
                .dstArrayElement = array_index,
                .descriptorType = descriptor_type,
            };
        }
    }

    void Renderer::bind_buffers(
        const DescriptorSetBlueprint* descriptor_set_blueprint,
        const std::span<const vk::DescriptorSet>& descriptor_sets,
        const std::span<const Buffer* const>& buffers,
        uint32_t binding_index,
        uint32_t array_index
    ) {
        std::vector<vk::DescriptorBufferInfo> buffer_infos(buffers.size());
        for (auto i = 0; i < buffers.size(); i++) {
            buffer_infos[i].buffer = buffers.data()[i]->buffer;
            buffer_infos[i].offset = 0;
            buffer_infos[i].range = buffers.data()[i]->size;
        }

        auto write_descriptor_set = make_descriptor_update(
            descriptor_set_blueprint,
            descriptor_sets,
            binding_index,
            array_index
        );
        write_descriptor_set.setBufferInfo(buffer_infos);

        m_device.updateDescriptorSets(write_descriptor_set, {});
    }

    void Renderer::bind_images(
        const DescriptorSetBlueprint* descriptor_set_blueprint,
        const std::span<const vk::DescriptorSet>& descriptor_sets,
        const std::span<const Image* const>& images,
        uint32_t binding_index,
        uint32_t array_index
    ) {
        std::vector<vk::DescriptorImageInfo> image_infos(images.size());
        for (auto i = 0; i < images.size(); i++) {
            image_infos[i].imageView = images.data()[i]->view;
            image_infos[i].imageLayout = vk::ImageLayout::eGeneral;
        }

        auto write_descriptor_set = make_descriptor_update(
            descriptor_set_blueprint,
            descriptor_sets,
            binding_index,
            array_index
        );
        write_descriptor_set.setImageInfo(image_infos);

        m_device.updateDescriptorSets(write_descriptor_set, {});
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Shaders                                                                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////

    ShaderModule* Renderer::create_shader_module(
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
        shader_module->reflection = std::make_unique<ShaderReflection>(spirv);
        return shader_module;
    }

    ShaderModule* Renderer::create_shader_module(const std::filesystem::path& path, bool optimize) {
        auto spirv = compile_glsl_to_spirv(path, optimize);
        const vk::ShaderModuleCreateInfo create_info {
            .codeSize = spirv_code_size(spirv),
            .pCode = spirv.data(),
        };

        auto shader_module = m_shader_module2_allocator.malloc();
        shader_module->module = m_device.createShaderModule(create_info);
        shader_module->reflection = std::make_unique<ShaderReflection>(spirv);
        return shader_module;
    }

    void Renderer::destroy_shader_module(ShaderModule* shader_module) {
        m_device.destroyShaderModule(shader_module->module);
        m_shader_module2_allocator.free(shader_module); // cleans up reflection
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Pipelines                                                                                //
    //////////////////////////////////////////////////////////////////////////////////////////////

    vk::PipelineLayout Renderer::create_pipeline_layout(
        const std::span<const ShaderStageInfo>& shader_modules,
        const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
    ) {
        std::vector<vk::PushConstantRange> push_constant_ranges;
        for (const auto& shader : shader_modules) {
            auto ranges = shader.module->reflection->get_push_constant_ranges();
            std::copy(ranges.begin(), ranges.end(), std::back_inserter(push_constant_ranges));
        }

        DescriptorSetArray<vk::DescriptorSetLayout> layouts = {
            descriptor_set_blueprints[0]->layout,
            descriptor_set_blueprints[1]->layout,
            descriptor_set_blueprints[2]->layout,
            descriptor_set_blueprints[3]->layout,
        };

        // If the graphicsPipelineLibrary feature is not enabled, elements of pSetLayouts must be
        // valid VkDescriptorSetLayout objects
        // So loop through layouts until the first NULL layout then use that index as the count
        uint32_t layout_count = 0;
        while ((layouts[layout_count] != VK_NULL_HANDLE) && (layout_count < layouts.size())) layout_count++;

        const vk::PipelineLayoutCreateInfo create_info {
            // Spec allows VK_NULL_HANDLES in the layout to indicate unused sets
            .setLayoutCount = layout_count,
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.data(),
        };
        return m_device.createPipelineLayout(create_info);
    }

    namespace {

        vk::PipelineShaderStageCreateInfo make_shader_stage_create_info(const Renderer::ShaderStageInfo& shader) {
            return vk::PipelineShaderStageCreateInfo {
                .stage = shader.module->reflection->get_shader_stage(),
                .module = shader.module->module,
                .pName = shader.entry_point,
                .pSpecializationInfo = shader.specialization_info
            };
        }

    }

    Pipeline* Renderer::create_compute_pipeline(
        const ShaderStageInfo& shader,
        const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
    ) {
        assert(shader.module != nullptr);
        assert(shader.entry_point != nullptr); // TODO: Could reflect this from the module as well

        auto pipeline = m_pipeline_allocator.malloc();
        pipeline->bind_point = vk::PipelineBindPoint::eCompute;

        pipeline->layout = create_pipeline_layout(std::array{shader}, descriptor_set_blueprints);

        const vk::ComputePipelineCreateInfo create_info {
            .stage = make_shader_stage_create_info(shader),
            .layout = pipeline->layout,
        };

        auto result = m_device.createComputePipeline(VK_NULL_HANDLE, create_info);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }
        pipeline->pipeline = result.value;

        return pipeline;
    }

    namespace {


        std::vector<vk::VertexInputAttributeDescription> get_vertex_attributes(
            const std::span<const Renderer::ShaderStageInfo>& stages
        ) {
            auto vertex = std::find_if(
                stages.begin(),
                stages.end(),
                [](const Renderer::ShaderStageInfo& stage) {
                    return stage.module->reflection->get_shader_stage() == vk::ShaderStageFlagBits::eVertex;
                }
            );

            if (vertex != stages.end()) {
                return vertex->module->reflection->get_input_attributes();
            } else {
                error(EMBER_GPU_LOG, "No vertex stage found when creating graphics pipeline!");
                throw std::runtime_error("No vertex stage found when creating graphics pipeline!");
            }
        }

        void patch_vertex_binding_indices(
            std::vector<vk::VertexInputAttributeDescription>& vertex_attributes,
            const std::span<const vk::VertexInputBindingDescription>& binding_descriptions
        ) {
            std::vector<vk::VertexInputBindingDescription> sorted_bindings(binding_descriptions.begin(), binding_descriptions.end());
            std::sort(
                sorted_bindings.begin(),
                sorted_bindings.end(),
                [](vk::VertexInputBindingDescription a, vk::VertexInputBindingDescription b) {
                    return a.binding < b.binding;
                }
            );

            uint32_t binding_offset = 0;
            for (const auto binding : sorted_bindings) {
                for (auto& attr : vertex_attributes) {
                    if (attr.offset >= binding_offset) attr.binding = binding.binding;
                }
                binding_offset += binding.stride;
            }
        }

        std::vector<vk::Format> get_color_attachment_formats(
            const std::span<const Renderer::ShaderStageInfo>& stages
        ) {
            auto fragment = std::find_if(
                stages.begin(),
                stages.end(),
                [](const Renderer::ShaderStageInfo& stage) {
                    return stage.module->reflection->get_shader_stage() == vk::ShaderStageFlagBits::eFragment;
                }
            );

            if (fragment != stages.end()) {
                return fragment->module->reflection->get_output_formats();
            } else {
                error(EMBER_GPU_LOG, "No fragment stage found when creating graphics pipeline!");
                throw std::runtime_error("No fragment stage found when creating graphics pipeline!");
            }
        }
    }

    Pipeline* Renderer::create_graphics_pipeline(
        const std::span<const ShaderStageInfo>& stages,
        const GraphicsPipelineState& pipeline_state,
        const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
    ) {
        auto pipeline = m_pipeline_allocator.malloc();
        pipeline->bind_point = vk::PipelineBindPoint::eGraphics;

        pipeline->layout = create_pipeline_layout(stages, descriptor_set_blueprints);

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(stages.size());
        for (const auto& stage : stages) {
            shader_stages.push_back(make_shader_stage_create_info(stage));
        }

        auto vertex_attributes = get_vertex_attributes(stages);
        patch_vertex_binding_indices(vertex_attributes, pipeline_state.vertex_bindings);

        const vk::PipelineVertexInputStateCreateInfo vertex_input_state {
            .vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline_state.vertex_bindings.size()),
            .pVertexBindingDescriptions = pipeline_state.vertex_bindings.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size()),
            .pVertexAttributeDescriptions = vertex_attributes.data()
        };

        const vk::PipelineViewportStateCreateInfo viewport_state{
            .viewportCount = 1,
            .scissorCount = 1
        };

        std::array dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamic_state {
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data(),
        };

        auto output_attachment_formats = get_color_attachment_formats(stages);
        const vk::PipelineRenderingCreateInfo rendering_info {
            .colorAttachmentCount = static_cast<uint32_t>(output_attachment_formats.size()),
            .pColorAttachmentFormats = output_attachment_formats.data(),
        };

        const vk::GraphicsPipelineCreateInfo create_info {
            .pNext = &pipeline_state.rendering_info,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .pVertexInputState = &vertex_input_state,
            .pInputAssemblyState = &pipeline_state.input_assembly_state,
            .pTessellationState = &pipeline_state.tesselation_state,
            .pViewportState = &viewport_state,
            .pRasterizationState = &pipeline_state.rasterization_state,
            .pMultisampleState = &pipeline_state.multisample_state,
            .pDepthStencilState = nullptr, // FIXME
            .pColorBlendState = &pipeline_state.color_blend_state,
            .pDynamicState = &dynamic_state,
            .layout = pipeline->layout
        };

        auto result = m_device.createGraphicsPipeline(VK_NULL_HANDLE, create_info);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }
        pipeline->pipeline = result.value;

        return pipeline;
    }

    void Renderer::destroy_pipeline(Pipeline* pipeline) {
        m_device.destroyPipeline(pipeline->pipeline);
        m_device.destroyPipelineLayout(pipeline->layout);
        m_pipeline_allocator.free(pipeline);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Presentation                                                                             //
    //////////////////////////////////////////////////////////////////////////////////////////////

    Swapchain* Renderer::create_swapchain_for_surface(
        vk::SurfaceKHR surface,
        vk::Extent2D window_extent,
        Swapchain* old_swapchain
    ) {
        auto swapchain = m_swapchain_allocator.malloc();

        vk::SwapchainKHR old_swapchain_khr = VK_NULL_HANDLE;
        if (old_swapchain != nullptr) old_swapchain_khr = old_swapchain->swapchain;
        auto create_info = m_gpu->get_swapchain_create_info_for_surface(surface, window_extent, old_swapchain_khr);

        swapchain->format = create_info.imageFormat;
        swapchain->extent = create_info.imageExtent;
        swapchain->swapchain = m_device.createSwapchainKHR(create_info);

        auto images = m_device.getSwapchainImagesKHR(swapchain->swapchain);
        swapchain->images.resize(images.size());

        for (auto i = 0; i < images.size(); i++) {
            auto& [image, semaphore] = swapchain->images[i];

            image = m_image_allocator.malloc();
            image->image = images[i];
            image->extent.width = swapchain->extent.width; // FIXME 2D <-> 3D extent conversion Plz
            image->extent.height = swapchain->extent.height;
            image->layout = vk::ImageLayout::eUndefined;

            const vk::ImageViewCreateInfo view_create_info{
                .image = image->image,
                .viewType = vk::ImageViewType::e2D,
                .format = create_info.imageFormat,
                .components = {
                    vk::ComponentSwizzle::eR,
                    vk::ComponentSwizzle::eG,
                    vk::ComponentSwizzle::eB,
                    vk::ComponentSwizzle::eA
                },
                .subresourceRange = MAX_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor)
            };
            image->view = m_device.createImageView(view_create_info);

            semaphore = create_semaphore();
        }

        if (old_swapchain != nullptr) destroy_swapchain(old_swapchain);
        return swapchain;
    }

    void Renderer::destroy_swapchain(Swapchain* swapchain) {
        for (auto [image, semaphore] : swapchain->images) {
            m_device.destroyImageView(image->view);
            destroy_semaphore(semaphore);
            m_image_allocator.free(image);
        }
        m_device.destroySwapchainKHR(swapchain->swapchain);
    }

    uint32_t Renderer::get_next_swapchain_image(
        const Swapchain* swapchain,
        vk::Semaphore wait_semaphore
    ) {
        auto res = m_device.acquireNextImageKHR(
            swapchain->swapchain,
            std::numeric_limits<uint64_t>::max(),
            wait_semaphore,
            VK_NULL_HANDLE
        );

        assert(res.result == vk::Result::eSuccess);
        return res.value;
    }

    void Renderer::present_swapchain(Swapchain* swapchain, uint32_t image_index) {
        auto& [image, render_semaphore] = swapchain->images.at(image_index);
        const vk::PresentInfoKHR present_info {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain->swapchain,
            .pImageIndices = &image_index
        };
        auto res = m_queue.presentKHR(present_info);
        assert(res == vk::Result::eSuccess);
        // FIXME: HANDLE THIS !!!
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Synchronization                                                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////

    void Renderer::wait_idle() {
        m_queue.waitIdle(); // or m_device.waitIdle(), not sure it matters when using one queue
    }

    vk::Fence Renderer::create_fence(bool signaled) {
        return m_device.createFence(vk::FenceCreateInfo{
            .flags = (signaled)?(vk::FenceCreateFlagBits::eSignaled):(vk::FenceCreateFlags{})
        });
    }

    void Renderer::destroy_fence(vk::Fence fence) {
        m_device.destroyFence(fence);
    }

    bool Renderer::wait_for_fences(const std::span<const vk::Fence>& fences, std::chrono::nanoseconds timeout) {
        auto result = m_device.waitForFences(
            fences.size(),
            fences.data(),
            VK_TRUE,
            timeout.count()
        );

        // FIXME: Should provide better handling of results in general
        return (result == vk::Result::eSuccess);
    }

    void Renderer::reset_fences(const std::span<const vk::Fence>& fences) {
        m_device.resetFences(fences);
    }

    vk::Semaphore Renderer::create_semaphore() {
        return m_device.createSemaphore({});
    }

    void Renderer::destroy_semaphore(vk::Semaphore semaphore) {
        m_device.destroySemaphore(semaphore);
    }
}