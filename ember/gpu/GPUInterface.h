#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>

#include "CommandRecorder.h"
#include "GPUDevice.h"
#include "RenderObjects.h"
#include "SPIRV.h"

#include "ember/util/Allocators.h"
#include "ember/util/ConstPtrSpan.h"

using ember::util::ConstPtrSpan;

namespace ember::gpu {

    struct BufferBindInfo {
        const Buffer* buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        BufferBindInfo(const Buffer* buffer): buffer(buffer), offset(0), size(vk::WholeSize) { }
        BufferBindInfo(const Buffer* buffer, vk::DeviceSize offset, vk::DeviceSize size): buffer(buffer), offset(offset), size(size) { }
    };

    class GPUInterface {
    public:
        GPUInterface(std::shared_ptr<const GPUDevice> device);
        ~GPUInterface();

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Command Buffers                                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Command buffer recording callback type
        using CommandRecordFn = std::function<void(CommandRecorder&)>;

        /// @brief Create a new command buffer
        /// @return Finished command buffer
        [[nodiscard]] vk::CommandBuffer create_command_buffer(
            vk::CommandBufferUsageFlags usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        );
        /// @brief Destroy a command buffer
        /// @param command_buffer
        void destroy_command_buffer(vk::CommandBuffer command_buffer);

        void restart_command_buffer(
            vk::CommandBuffer command_buffer,
            vk::CommandBufferUsageFlags usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        );

        void record_command_buffer(vk::CommandBuffer command_buffer, const CommandRecordFn& fn);

        /// @brief Record and submit a command buffer
        /// @param fn Recording callback
        void record_submit_command_buffer(const CommandRecordFn& fn, vk::Fence fence = VK_NULL_HANDLE);

        /// @brief Submit command buffers to the GPU
        /// @param command_buffers Command buffers to submit
        /// @param wait_semaphores Semaphores to wait on before commands are executed
        /// @param signal_sempahores Semaphores to signal when commands complete
        /// @return Fence indicating command buffers have finished
        vk::Fence submit_command_buffers(
            const std::span<const vk::CommandBuffer>& command_buffers,
            vk::Fence fence = VK_NULL_HANDLE,
            const std::span<const vk::Semaphore>& wait_semaphores = {},
            const std::span<const vk::PipelineStageFlags>& dst_stage_mask = {},
            const std::span<const vk::Semaphore>& signal_sempahores = {}
        );
        /// @brief Submit a command buffer to the GPU
        /// @param command_buffers Command buffer to submit
        /// @param wait_semaphores Semaphores to wait on before commands are executed
        /// @param signal_sempahores Semaphores to signal when commands complete
        /// @return Fence indicating command buffer has finished
        vk::Fence submit_command_buffer(
            vk::CommandBuffer command_buffer,
            vk::Fence fence = VK_NULL_HANDLE,
            const std::span<const vk::Semaphore>& wait_semaphores = {},
            const std::span<const vk::PipelineStageFlags>& dst_stage_mask = {},
            const std::span<const vk::Semaphore>& signal_sempahores = {}
        );

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Memory                                                                                   //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Create a new buffer
        /// @param size Size in bytes
        /// @param usage Buffer usage
        /// @param properties Memory properties
        /// @return Allocated buffer
        Buffer* create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        inline Buffer* create_device_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
            return create_buffer(size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
        }
        inline Buffer* create_storage_buffer(vk::DeviceSize size, vk::MemoryPropertyFlags properties) {
            return create_buffer(size, vk::BufferUsageFlagBits::eStorageBuffer, properties);
        }
        inline Buffer* create_vertex_buffer(vk::DeviceSize size) {
            return create_device_buffer(
                size,
                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
            );
        }
        inline Buffer* create_index_buffer(vk::DeviceSize size) {
            return create_device_buffer(
                size,
                vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
            );
        }
        inline Buffer* create_uniform_buffer(vk::DeviceSize size) {
            return create_device_buffer(
                size,
                vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst
            );
        }

        /// @brief Destroy a previously allocated buffer
        /// @param buffer
        void destroy_buffer(Buffer* buffer);

        /// @brief Read the contents of a buffer
        /// @param dst Destination memory address
        /// @param buffer Buffer to read
        /// @param offset Offset within buffer
        /// @param size Number of bytes to read
        void read_buffer(void* dst, const Buffer* buffer, vk::DeviceSize offset, vk::DeviceSize size = vk::WholeSize) const;
        void download_buffer(void* dst, const Buffer* buffer, vk::DeviceSize offset, vk::DeviceSize size  = vk::WholeSize);

        /// @brief Write data to a buffer
        /// @param buffer Buffer to be written
        /// @param src Source data address
        /// @param offset Offset within buffer (default = 0)
        /// @param size Number of bytes to write (default = buffer->size)
        void write_buffer(Buffer* buffer, const void* src, vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize);
        void upload_buffer(Buffer* buffer, const void* src, vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize);

        template<typename T>
        void write_buffer(Buffer* buffer, const std::span<const T>& data, vk::DeviceSize offset = 0) {
            write_buffer(buffer, data.data(), offset, data.size() * sizeof(T));
        }
        template<typename T>
        void upload_buffer(Buffer* buffer, const std::span<const T>& data, vk::DeviceSize offset = 0) {
            upload_buffer(buffer, data.data(), offset, data.size() * sizeof(T));
        }

        template<class Iter>
        void write_buffer(Buffer* buffer, const Iter begin, const Iter end, vk::DeviceSize offset = 0) {
            auto dst = static_cast<Iter::value_type*>(m_device.mapMemory(buffer->memory, offset, buffer->size));
            for (auto iter = begin; iter != end; iter++) {
                *dst = *iter;
                dst++;
            }
            m_device.unmapMemory(buffer->memory);
        }
        template<class Iter>
        void upload_buffer(Buffer* buffer, const Iter begin, const Iter end, vk::DeviceSize offset = 0) {
            // auto dst = static_cast<Iter::value_type*>(m_device.mapMemory(buffer->memory, offset, buffer->size));
            // for (auto iter = begin; iter != end; iter++) {
            //     *dst = *iter;
            //     dst++;
            // }
            // m_device.unmapMemory(buffer->memory);
        }

        struct ImageCreateInfo {
            vk::ImageType type;
            vk::Format format;
            vk::Extent3D extent;
            vk::ImageUsageFlags usage;
            vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
            vk::ImageLayout layout;
            vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
            vk::MemoryPropertyFlags memory_properties;
        };
        Image* create_image(const ImageCreateInfo& image_info);
        void destroy_image(Image* image);
        void read_image(void* dst, const Image* image);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Descriptor Sets                                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////

        DescriptorSetArray<DescriptorSetBlueprint*> create_descriptor_set_blueprints(
            const ConstPtrSpan<ShaderModule>& shader_stages
        );

        void destroy_descriptor_set_blueprints(const std::span<DescriptorSetBlueprint*>& blueprints);

        std::vector<vk::DescriptorSet> create_descriptor_sets(
            DescriptorSetBlueprint* descriptor_set_blueprint,
            uint32_t num_descriptor_sets
        );

        void destroy_descriptor_sets(
            DescriptorSetBlueprint* descriptor_set_blueprint,
            const std::span<const vk::DescriptorSet>& descriptor_sets
        );

        void bind_buffers(
            const DescriptorSetBlueprint* descriptor_set_blueprint,
            const std::span<const vk::DescriptorSet>& descriptor_sets,
            const ConstPtrSpan<Buffer>& buffers,
            uint32_t binding_index,
            uint32_t array_index = 0
        );
        void bind_images(
            const DescriptorSetBlueprint* descriptor_set_blueprint,
            const std::span<const vk::DescriptorSet>& descriptor_sets,
            const ConstPtrSpan<Image>& images,
            uint32_t binding_index,
            uint32_t array_index = 0
        );

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Shaders                                                                                  //
        //////////////////////////////////////////////////////////////////////////////////////////////

        ShaderModule* create_shader_module(
            const std::string& glsl,
            shaderc_shader_kind shader_kind,
            const std::string& filename = "",
            bool optimize = false
        );
        ShaderModule* create_shader_module(const std::filesystem::path& path, bool optimize = false);
        void destroy_shader_module(ShaderModule* shader_module);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Pipelines                                                                                //
        //////////////////////////////////////////////////////////////////////////////////////////////

        struct ShaderStageInfo {
            const ShaderModule* module;
            const char* entry_point = "main";
            const vk::SpecializationInfo* specialization_info = nullptr;
        };
        Pipeline* create_compute_pipeline(
            const ShaderStageInfo& shader,
            const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
        );
        struct GraphicsPipelineState {
            std::span<const vk::VertexInputBindingDescription> vertex_bindings;
            vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
            vk::PipelineTessellationStateCreateInfo tesselation_state;
            vk::PipelineRasterizationStateCreateInfo rasterization_state;
            vk::PipelineMultisampleStateCreateInfo multisample_state;
            const vk::PipelineDepthStencilStateCreateInfo* depth_stencil_state = nullptr;
            vk::PipelineColorBlendStateCreateInfo color_blend_state;
            vk::PipelineRenderingCreateInfo rendering_info;
        };
        Pipeline* create_graphics_pipeline(
            const std::span<const ShaderStageInfo>& stages,
            const GraphicsPipelineState& pipeline_state,
            const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
        );

        /// @brief Destroy a pipeline of any type
        /// @param pipeline
        void destroy_pipeline(Pipeline* pipeline);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Presentation                                                                             //
        //////////////////////////////////////////////////////////////////////////////////////////////

        Swapchain* create_swapchain_for_surface(
            vk::SurfaceKHR surface,
            vk::Extent2D window_extent,
            Swapchain* old_swapchain = VK_NULL_HANDLE
        );
        void destroy_swapchain(Swapchain* swapchain);

        // FIXME should probably handle any non-success returns internally
        uint32_t get_next_swapchain_image(
            const Swapchain* swapchain,
            vk::Semaphore wait_semaphore
        );

        void begin_rendering_to_swapchain(
            Swapchain* swapchain,
            uint32_t image_index,
            vk::CommandBuffer command_buffer,
            const vk::ClearValue& clear_value
        );
        void end_rendering_to_swapchain(
            Swapchain* swapchain,
            uint32_t image_index,
            vk::CommandBuffer command_buffer
        );
        void present_swapchain(Swapchain* swapchain, uint32_t image_index);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Synchronization                                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////

        void wait_idle();

        vk::Fence create_fence(bool signaled = false);
        void destroy_fence(vk::Fence fence);

        /// @brief Wait for a list of fences to complete
        /// @param fences List of fences to wait on
        /// @param timeout Wait timeout
        /// @return true if all fences completed, false otherwise
        bool wait_for_fences(
            const std::span<const vk::Fence>& fences,
            std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()
        );

        void reset_fences(const std::span<const vk::Fence>& fences);

        /// @brief Create a sempahore
        /// @return Created sempahore
        [[nodiscard]] vk::Semaphore create_semaphore();

        /// @brief Destroy a semaphore
        /// @param semaphore
        void destroy_semaphore(vk::Semaphore semaphore);

    private:
        std::shared_ptr<const GPUDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;

        vk::CommandPool m_command_pool;

        template<typename T>
        using ObjectAllocator = util::PoolAllocator<T>;
        ObjectAllocator<Buffer> m_buffer_allocator;
        ObjectAllocator<Image> m_image_allocator;
        ObjectAllocator<Pipeline> m_pipeline_allocator;
        ObjectAllocator<ShaderModule> m_shader_module2_allocator;
        ObjectAllocator<DescriptorSetBlueprint> m_descriptor_set_blueprint_allocator;
        ObjectAllocator<Swapchain> m_swapchain_allocator;

        vk::DeviceMemory allocate_memory(
            vk::MemoryRequirements requirements,
            vk::MemoryPropertyFlags properties
        );

        vk::PipelineLayout create_pipeline_layout(
            const std::span<const ShaderStageInfo>& shader_modules,
            const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
        );

        vk::CommandBuffer allocate_command_buffer(
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
        );
    };

}