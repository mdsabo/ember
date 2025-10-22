#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>

#include "CommandRecorder.h"
#include "GraphicsDevice.h"
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

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        vk::SwapchainKHR create_swapchain_for_surface(
            vk::SurfaceKHR surface,
            vk::Extent2D window_extent,
            vk::SwapchainKHR old_swapchain = VK_NULL_HANDLE
        );

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

        void record_command_buffer(vk::CommandBuffer command_buffer, const CommandRecordFn& fn);

        /// @brief Record and submit a command buffer
        /// @param fn Recording callback
        void record_submit_command_buffer(const CommandRecordFn& fn);

        /// @brief Submit command buffers to the GPU
        /// @param command_buffers Command buffers to submit
        /// @param wait_semaphores Semaphores to wait on before commands are executed
        /// @param signal_sempahores Semaphores to signal when commands complete
        /// @return Fence indicating command buffers have finished
        [[nodiscard]] vk::Fence submit_command_buffers(
            const std::span<const vk::CommandBuffer>& command_buffers,
            const std::span<const vk::Semaphore>& wait_semaphores = {},
            const std::span<const vk::Semaphore>& signal_sempahores = {}
        );
        /// @brief Submit a command buffer to the GPU
        /// @param command_buffers Command buffer to submit
        /// @param wait_semaphores Semaphores to wait on before commands are executed
        /// @param signal_sempahores Semaphores to signal when commands complete
        /// @return Fence indicating command buffer has finished
        [[nodiscard]] vk::Fence submit_command_buffer(
            vk::CommandBuffer command_buffer,
            const std::span<const vk::Semaphore>& wait_semaphores = {},
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

        /// @brief Destroy a previously allocated buffer
        /// @param buffer
        void destroy_buffer(Buffer* buffer);

        /// @brief Read the contents of a buffer
        /// @param dst Destination memory address
        /// @param buffer Buffer to read
        /// @param offset Offset within buffer
        /// @param size Number of bytes to read
        void read_buffer(void* dst, const Buffer* buffer, vk::DeviceSize offset, vk::DeviceSize size) const;

        /// @brief Write data to a buffer
        /// @param buffer Buffer to be written
        /// @param src Source data address
        /// @param offset Offset within buffer
        /// @param size Number of bytes to write
        void write_buffer(Buffer* buffer, const void* src, vk::DeviceSize offset, vk::DeviceSize size);

        template<class Iter>
        void write_buffer(Buffer* buffer, const Iter begin, const Iter end, vk::DeviceSize offset = 0) {
            auto dst = static_cast<Iter::value_type*>(m_device.mapMemory(buffer->memory, offset, buffer->size));
            for (auto iter = begin; iter != end; iter++) {
                *dst = *iter;
                dst++;
            }
            m_device.unmapMemory(buffer->memory);
        }

        struct ImageCreateInfo {
            vk::ImageType type;
            vk::Format format;
            vk::Extent3D extent;
            vk::ImageUsageFlags usage;
            vk::ImageLayout layout;
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
            const std::string& filename = "",
            shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source, // #pragma shader_stage(something)
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
        Pipeline* create_graphics_pipeline(
            const std::span<const ShaderStageInfo>& stages,
            const vk::PipelineVertexInputStateCreateInfo& vertex_input_state,
            const vk::PipelineInputAssemblyStateCreateInfo& input_assembly_state,
            const vk::PipelineTessellationStateCreateInfo& tesselation_state,
            const vk::PipelineViewportStateCreateInfo& viewport_state,
            const vk::PipelineRasterizationStateCreateInfo& rasterization_state,
            const vk::PipelineMultisampleStateCreateInfo& multisample_state,
            const vk::PipelineDepthStencilStateCreateInfo& depth_stencil_state,
            const vk::PipelineColorBlendStateCreateInfo& color_blend_state,
            const vk::PipelineDynamicStateCreateInfo& dynamic_state,
            const vk::PipelineRenderingCreateInfo& rendering_info,
            const DescriptorSetArray<DescriptorSetBlueprint*>& descriptor_set_blueprints
        );

        /// @brief Destroy a pipeline of any type
        /// @param pipeline
        void destroy_pipeline(Pipeline* pipeline);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Synchronization                                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Wait for a list of fences to complete
        /// @param fences List of fences to wait on
        /// @param timeout Wait timeout
        /// @return true if all fences completed, false otherwise
        bool wait_for_fences(
            const std::span<const vk::Fence>& fences,
            std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()
        );

        /// @brief Create a sempahore
        /// @return Created sempahore
        [[nodiscard]] vk::Semaphore create_semaphore();

        /// @brief Destroy a semaphore
        /// @param semaphore
        void destroy_semaphore(vk::Semaphore semaphore);

    private:
        std::shared_ptr<const GraphicsDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;

        vk::CommandPool m_command_pool;
        unsigned int m_allocated_command_buffers;

        template<typename T>
        using ObjectAllocator = util::SlabAllocator<T>;
        ObjectAllocator<Buffer> m_buffer_allocator;
        ObjectAllocator<Image> m_image_allocator;
        ObjectAllocator<Pipeline> m_pipeline_allocator;
        ObjectAllocator<ShaderModule> m_shader_module2_allocator;
        ObjectAllocator<DescriptorSetBlueprint> m_descriptor_set_blueprint_allocator;

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