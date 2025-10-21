#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string_view>

#include "CommandRecorder.h"
#include "GraphicsDevice.h"
#include "RenderObjects.h"
#include "SPIRV.h"

#include "ember/util/Allocators.h"

namespace ember::gpu {

    using vk::ArrayProxy;

    // FIXME: Where should I put these?
    struct DescriptorWrite {
        uint32_t set_index = 0;
        uint32_t binding_index = 0;
        union {
            uint32_t array_index = 0;
            // If the descriptor binding identified by dstSet and dstBinding has a descriptor type of
            // VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK then dstArrayElement specifies the starting byte
            // offset within the binding.
            uint32_t inline_uniform_block_byte_index;
        };
    };

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
            const ArrayProxy<vk::CommandBuffer>& command_buffers,
            const ArrayProxy<vk::Semaphore>& wait_semaphores = {},
            const ArrayProxy<vk::Semaphore>& signal_sempahores = {}
        );
        /// @brief Submit a command buffer to the GPU
        /// @param command_buffers Command buffer to submit
        /// @param wait_semaphores Semaphores to wait on before commands are executed
        /// @param signal_sempahores Semaphores to signal when commands complete
        /// @return Fence indicating command buffer has finished
        [[nodiscard]] vk::Fence submit_command_buffer(
            vk::CommandBuffer command_buffer,
            const ArrayProxy<vk::Semaphore>& wait_semaphores = {},
            const ArrayProxy<vk::Semaphore>& signal_sempahores = {}
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

        /// @brief Bind buffers to descriptors for later use
        /// @param shader_module Shader module of the descriptors
        /// @param descriptor_sets Descriptor sets to bind to
        /// @param descriptor_write Indicies within the descriptor set to write
        /// @param buffers Buffers to bind
        void bind_buffers(
            const ShaderModule* shader_module,
            const DescriptorSets* descriptor_sets,
            const DescriptorWrite& descriptor_write,
            const ArrayProxy<Buffer*>& buffers
        );

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
        /// @brief Bind images to descriptors for later use
        /// @param shader_module Shader module of the descriptors
        /// @param descriptor_sets Descriptor sets to bind to
        /// @param descriptor_write Indicies within the descriptor set to write
        /// @param buffers Images to bind
        void bind_images(
            const ShaderModule* shader_module,
            const DescriptorSets* descriptor_sets,
            const DescriptorWrite& descriptor_write,
            const ArrayProxy<Image*>& images
        );

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Descriptor Sets                                                                          //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Create descriptor sets that can be used during shader execution
        /// @param shader_module Target shader module
        /// @param set_index Descriptor set index
        /// @param descriptor_set_count Number of descriptor sets to create
        /// @return Allocated block of descriptor sets
        DescriptorSets* create_descriptor_sets(ShaderModule* shader_module, uint32_t set_index, uint32_t descriptor_set_count);

        /// @brief Destory a block fo descriptor sets
        /// @param descriptor_sets Descriptor sets to destroy
        void destroy_descriptor_sets(DescriptorSets* descriptor_sets);

        /// @brief Merge two descriptor set allocations
        ///
        /// The two sets MUST have the same set index and shader module
        ///
        /// @param dst Merge sets into this allocation
        /// @param src Destroy this allocation after transferring sets
        void merge_descriptor_sets(DescriptorSets* dst, DescriptorSets* src);




        DescriptorSetAllocator* create_descriptor_set_allocator(
            const ArrayProxy<ShaderModule2*>& shader_stages
        );

        void destroy_descriptor_set_allocator(DescriptorSetAllocator* dsa);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Shaders                                                                                  //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Create a shader module from a given path
        /// @param path Path to GLSL shader file
        /// @return Created shader module
        ShaderModule* create_shader_module(const std::filesystem::path& path);

        /// @brief Destroy a shader module
        /// @param module
        void destroy_shader_module(ShaderModule* module);




        ShaderModule2* create_shader_module2(
            const std::string& glsl,
            const std::string& filename = "",
            shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source, // #pragma shader_stage(something)
            bool optimize = false
        );
        ShaderModule2* create_shader_module2(const std::filesystem::path& path, bool optimize = false);
        void destroy_shader_module2(ShaderModule2* shader_module);

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Pipelines                                                                                //
        //////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Create a compute pipeline
        /// @param shader_module Shader to be run by the pipeline
        /// @param entry_point Name of the shader entry point (default: "main")
        /// @return Created pipeline
        Pipeline* create_compute_pipeline(
            const ShaderModule* shader_module,
            const std::string_view& entry_point = "main"
        );
        Pipeline* create_compute_pipeline2(
            ShaderModule2* shader,
            const char* entry_point = "main",
            vk::SpecializationInfo* specialization_info = nullptr,
            DescriptorSetAllocator* descriptor_set_allocator = nullptr
        );
        Pipeline* create_graphics_pipeline(
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
            const ArrayProxy<vk::Fence>& fences,
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

        util::SlabAllocator<Buffer> m_buffer_allocator;
        util::SlabAllocator<Image> m_image_allocator;
        util::SlabAllocator<Pipeline> m_pipeline_allocator;
        util::SlabAllocator<ShaderModule> m_shader_module_allocator;
        util::SlabAllocator<DescriptorSets> m_descriptor_sets_allocator;


        util::SlabAllocator<ShaderModule2, 16> m_shader_module2_allocator;
        util::SlabAllocator<DescriptorSetAllocator> m_descriptor_set_allocator_allocator;

        vk::DeviceMemory allocate_memory(
            vk::MemoryRequirements requirements,
            vk::MemoryPropertyFlags properties
        );

        void init_descriptor_pools(
            DescriptorSetArray<DescriptorPool>& descriptor_pools,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
        );

        vk::PipelineLayout create_pipeline_layout(const ShaderModule* shader_module);

        vk::CommandBuffer allocate_command_buffer(
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
        );
    };

}