#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string_view>

#include "CommandRecorder.h"
#include "GraphicsDevice.h"
#include "RenderObjects.h"

namespace ember::gpu {

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
        const Buffer& buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;

        BufferBindInfo(const Buffer& buffer): buffer(buffer), offset(0), size(vk::WholeSize) { }
        BufferBindInfo(const Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size): buffer(buffer), offset(offset), size(size) { }
    };

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        using CommandRecordFn = std::function<void(CommandRecorder&)>;
        CommandBuffer record_command_buffer(bool one_time_submit, const CommandRecordFn& fn);
        void destroy_command_buffer(CommandBuffer& command_buffer);
        Fence submit_command_buffers(
            std::vector<CommandBuffer> command_buffers,
            const std::vector<Semaphore>& wait_semaphores = {},
            const std::vector<Semaphore>& signal_sempahores = {}
        );
        Fence submit_command_buffer(
            CommandBuffer command_buffer,
            const std::vector<Semaphore>& wait_semaphores = {},
            const std::vector<Semaphore>& signal_sempahores = {}
        );

        Buffer create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void destroy_buffer(Buffer& buffer);
        void read_buffer(void* dst, const Buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size) const;
        void write_buffer(Buffer& buffer, void* src, vk::DeviceSize offset, vk::DeviceSize size);
        void bind_buffers(
            const ShaderModule& shader_module,
            const DescriptorSetChunk& descriptor_sets, 
            const DescriptorWrite& descriptor_write, 
            const std::vector<BufferBindInfo>& buffers
        );

        DescriptorSetChunk create_descriptor_sets(ShaderModule& shader_module, uint32_t set_index, uint32_t descriptor_set_count);
        void destroy_descriptor_sets(ShaderModule& shader_module, DescriptorSetChunk& descriptor_sets);

        ShaderModule create_shader_module(const std::filesystem::path& path);
        void destroy_shader_module(ShaderModule& module);

        Pipeline create_compute_pipeline(const ShaderModule& shader_module, const std::string_view& entry_point = "main");
        void destroy_pipeline(Pipeline& pipeline);

        bool wait_for_fences(
            std::vector<Fence> fences, 
            std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()
        );

        Semaphore create_semaphore();
        void destroy_semaphore(Semaphore& semaphore);

    private:
        std::shared_ptr<const GraphicsDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;

        vk::CommandPool m_command_pool;
        unsigned int m_allocated_command_buffers;

        vk::DeviceMemory allocate_memory(
            vk::DeviceSize size,
            vk::MemoryRequirements requirements,
            vk::MemoryPropertyFlags properties
        );


        std::vector<ShaderModule::DescriptorSetAllocationInfo> create_descriptor_set_allocation_infos(
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
        );

        vk::PipelineLayout create_pipeline_layout(const ShaderModule& shader_module);
    };

}