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

    // CommandBuffer Renderer::record_command_buffer(const CommandRecordFn& fn) {
    //     auto command_buffer = allocate_command_buffer();

    //     command_buffer.begin({});

    //     auto recorder = CommandRecorder(command_buffer);
    //     fn(recorder);

    //     command_buffer.end();
    //     return CommandBuffer(command_buffer);
    // }

    CommandBuffer Renderer::create_command_buffer(vk::CommandBufferUsageFlags usage) {
        auto command_buffer = allocate_command_buffer();
        command_buffer.begin({ .flags = usage });
        return command_buffer;
    }

    void Renderer::destroy_command_buffer(CommandBuffer&& command_buffer) {
        m_device.freeCommandBuffers(m_command_pool, command_buffer);
        command_buffer = VK_NULL_HANDLE;
    }

    void Renderer::record_command_buffer(CommandBuffer& command_buffer, const CommandRecordFn& fn) {
        auto recorder = CommandRecorder(command_buffer, m_gpu->queue_family_index());
        fn(recorder);
    }

    void Renderer::record_submit_command_buffer(const CommandRecordFn& fn) {
        auto command_buffer = create_command_buffer();
        record_command_buffer(command_buffer, fn);
        auto fence = submit_command_buffer(command_buffer);
        wait_for_fences({ fence });
        destroy_command_buffer(std::move(command_buffer));
    }

    Fence Renderer::submit_command_buffers(
        const ArrayProxy<CommandBuffer>& command_buffers,
        const ArrayProxy<Semaphore>& wait_semaphores,
        const ArrayProxy<Semaphore>& signal_sempahores
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
        return Fence(fence);
    }

    Fence Renderer::submit_command_buffer(
        CommandBuffer command_buffer,
        const ArrayProxy<Semaphore>& wait_semaphores,
        const ArrayProxy<Semaphore>& signal_sempahores
    ) {
        return submit_command_buffers(command_buffer, wait_semaphores, signal_sempahores);
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

    Buffer Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        const vk::BufferCreateInfo buffer_create_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        auto buffer = m_device.createBuffer(buffer_create_info);
        auto memory_requirements = m_device.getBufferMemoryRequirements(buffer);
        auto memory = allocate_memory(memory_requirements, properties);
        m_device.bindBufferMemory(buffer, memory, 0);

        return Buffer(size, buffer, memory);
    }

    void Renderer::destroy_buffer(Buffer&& buffer) {
        m_device.freeMemory(buffer.memory);
        buffer.memory = VK_NULL_HANDLE;
        m_device.destroyBuffer(buffer.buffer);
        buffer.buffer = VK_NULL_HANDLE;
    }

    void Renderer::read_buffer(void* dst, const Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size) const {
        auto src = m_device.mapMemory(buffer.memory, offset, size);
        memcpy(dst, src, size);
        m_device.unmapMemory(buffer.memory);
    }

    void Renderer::write_buffer(Buffer& buffer, void* src, vk::DeviceSize offset, vk::DeviceSize size) {
        auto dst = m_device.mapMemory(buffer.memory, offset, size);
        memcpy(dst, src, size);
        m_device.unmapMemory(buffer.memory);
    }

    void Renderer::bind_buffers(
        const ShaderModule& shader_module,
        const DescriptorSetChunk& descriptor_sets,
        const DescriptorWrite& descriptor_write,
        const ArrayProxy<BufferBindInfo>& buffers
    ) {
        std::vector<vk::DescriptorBufferInfo> buffer_infos(buffers.size());
        for (auto i = 0; i < buffers.size(); i++) {
            buffer_infos[i].buffer = buffers.data()[i].buffer.buffer;
            buffer_infos[i].offset = buffers.data()[i].offset;
            buffer_infos[i].range = buffers.data()[i].size;
        }

        const vk::WriteDescriptorSet write_descriptor_set {
            .dstSet = descriptor_sets.sets.at(descriptor_write.set_index),
            .dstBinding = descriptor_write.binding_index,
            .dstArrayElement = descriptor_write.array_index,
            .descriptorCount = buffers.size(),
            .descriptorType = shader_module.get_descriptor_type(descriptor_sets.set_index, descriptor_write.binding_index),
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

    Image Renderer::create_image(const ImageCreateInfo& image_info) {
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

        auto res = Image(image_info.extent, vk::ImageLayout::eUndefined, image, view, memory);

        record_submit_command_buffer([&res, &image_info](CommandRecorder& recorder) {
            recorder.transition_image_layout(res, image_info.layout);
        });

        return res;
    }

    void Renderer::destroy_image(Image&& image) {
        m_device.destroyImageView(image.view);
        image.view = VK_NULL_HANDLE;
        m_device.freeMemory(image.memory);
        image.memory = VK_NULL_HANDLE;
        m_device.destroyImage(image.image);
        image.image = VK_NULL_HANDLE;
    }

    void Renderer::read_image(void* dst, const Image& image) {
        auto properties = m_device.getImageMemoryRequirements(image.image);
        auto src = m_device.mapMemory(image.memory, 0, properties.size);
        memcpy(dst, src, properties.size);
        m_device.unmapMemory(image.memory);
    }

    void Renderer::bind_images(
        const ShaderModule& shader_module,
        const DescriptorSetChunk& descriptor_sets,
        const DescriptorWrite& descriptor_write,
        const ArrayProxy<Image>& images
    ) {
        std::vector<vk::DescriptorImageInfo> image_infos(images.size());
        for (auto i = 0; i < images.size(); i++) {
            image_infos[i].imageView = images.data()[i].view;
            image_infos[i].imageLayout = vk::ImageLayout::eGeneral;
        }

        const vk::WriteDescriptorSet write_descriptor_set {
            .dstSet = descriptor_sets.sets.at(descriptor_write.set_index),
            .dstBinding = descriptor_write.binding_index,
            .dstArrayElement = descriptor_write.array_index,
            .descriptorCount = images.size(),
            .descriptorType = shader_module.get_descriptor_type(descriptor_sets.set_index, descriptor_write.binding_index),
            .pImageInfo = image_infos.data(),
        };

        m_device.updateDescriptorSets(write_descriptor_set, {});
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

    void Renderer::destroy_descriptor_sets(ShaderModule& shader_module, DescriptorSetChunk&& descriptor_sets) {
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

    void Renderer::destroy_shader_module(ShaderModule&& module) {
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

    void Renderer::destroy_pipeline(Pipeline&& pipeline) {
        m_device.destroyPipeline(pipeline.pipeline);
        pipeline.pipeline = VK_NULL_HANDLE;
        m_device.destroyPipelineLayout(pipeline.layout);
        pipeline.layout = VK_NULL_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Fences/Semaphores                                                                        //
    //////////////////////////////////////////////////////////////////////////////////////////////

    bool Renderer::wait_for_fences(const ArrayProxy<Fence>& fences, std::chrono::nanoseconds timeout) {
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


    Semaphore Renderer::create_semaphore() {
        auto semaphore = m_device.createSemaphore({});
        return Semaphore(semaphore);
    }

    void Renderer::destroy_semaphore(Semaphore&& semaphore) {
        m_device.destroySemaphore(semaphore);
        semaphore = VK_NULL_HANDLE;
    }
}