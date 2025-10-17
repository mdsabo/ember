#pragma once

#include <filesystem>
#include <string_view>

#include "Buffer.h"
#include "DescriptorPool.h"
#include "GraphicsDevice.h"
#include "Pipeline.h"
#include "ShaderModule.h"

namespace ember::gpu {

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        Buffer create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void destroy_buffer(Buffer& buffer);

        ShaderModule create_shader_module(const std::filesystem::path& path);
        void destroy_shader_module(ShaderModule& module);

        Pipeline create_compute_pipeline(const ShaderModule& shader_module, const std::string_view& entry_point = "main");
        void destroy_pipeline(Pipeline& pipeline);

    private:
        std::shared_ptr<const GraphicsDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;

        DescriptorPool m_descriptor_pool;

        vk::DeviceMemory allocate_memory(
            vk::DeviceSize size,
            vk::MemoryRequirements requirements,
            vk::MemoryPropertyFlags properties
        );

        vk::PipelineLayout create_pipeline_layout(const ShaderModule& shader_module);
    };

}