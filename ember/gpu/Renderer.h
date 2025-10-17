#pragma once

#include <filesystem>
#include <functional>
#include <string_view>

#include "Buffer.h"
#include "CommandRecorder.h"
#include "DescriptorSetChunk.h"
#include "GraphicsDevice.h"
#include "Pipeline.h"
#include "ShaderModule.h"

namespace ember::gpu {

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        using CommandRecordFn = std::function<void(CommandRecorder&)>;
        vk::CommandBuffer record_render_commands(const CommandRecordFn& fn);
        void submit_command_buffers(const std::vector<const vk::CommandBuffer>& command_buffers);
        void submit_command_buffer(const vk::CommandBuffer command_buffer);

        Buffer create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void destroy_buffer(Buffer& buffer);

        DescriptorSetChunk create_descriptor_sets(ShaderModule& shader_module, uint32_t set_index, uint32_t descriptor_set_count);
        void destroy_descriptor_sets(DescriptorSetChunk& descriptor_sets);

        ShaderModule create_shader_module(const std::filesystem::path& path);
        void destroy_shader_module(ShaderModule& module);

        Pipeline create_compute_pipeline(const ShaderModule& shader_module, const std::string_view& entry_point = "main");
        void destroy_pipeline(Pipeline& pipeline);

    private:
        std::shared_ptr<const GraphicsDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;
        vk::CommandPool m_command_pool;
        
        vk::DeviceMemory allocate_memory(
            vk::DeviceSize size,
            vk::MemoryRequirements requirements,
            vk::MemoryPropertyFlags properties
        );


        std::vector<vk::DescriptorSetLayout> create_descriptor_set_layouts(
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
        );
        std::vector<ShaderModule::DescriptorPool> create_descriptor_pools(
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings
        );

        vk::PipelineLayout create_pipeline_layout(const ShaderModule& shader_module);
    };

}