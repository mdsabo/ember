#pragma once

#include <filesystem>
#include <string_view>

#include "Buffer.h"
#include "GraphicsDevice.h"
#include "Pipeline.h"
#include "ShaderModule.h"

namespace ember::gpu {

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        static constexpr uint32_t DEFAULT_DESCRIPTOR_POOL_SIZE = 1024;
        struct DescriptorPoolSizes {
            // VK_DESCRIPTOR_TYPE_SAMPLER = 0,
            uint32_t samplers = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
            uint32_t combined_image_samplers = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
            uint32_t sampled_image = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
            uint32_t uniform_storage_image = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
            uint32_t uniform_texel_buffer = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
            uint32_t storage_texel_buffer = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
            uint32_t uniform_buffer = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
            uint32_t storage_buffer = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
            uint32_t uniform_buffer_dynamic = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
            uint32_t storage_buffer_dynamic = DEFAULT_DESCRIPTOR_POOL_SIZE;
            // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
            uint32_t input_attachment = DEFAULT_DESCRIPTOR_POOL_SIZE;

            // VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK = 1000138000,
            // VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000,
            // VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV = 1000165000,
            // VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM = 1000440000,
            // VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM = 1000440001,
            // VK_DESCRIPTOR_TYPE_TENSOR_ARM = 1000460000,
            // VK_DESCRIPTOR_TYPE_MUTABLE_EXT = 1000351000,
            // VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV = 1000570000,
            // VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
            // VK_DESCRIPTOR_TYPE_MUTABLE_VALVE = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
        };
        void set_descriptor_pool_sizes(const DescriptorPoolSizes& pool_sizes);
        DescriptorPoolSizes get_max_descriptor_pool_usage();

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

        vk::DeviceMemory allocate_memory(
            vk::DeviceSize size,
            vk::MemoryRequirements requirements, 
            vk::MemoryPropertyFlags properties
        );

        vk::PipelineLayout create_pipeline_layout(const ShaderModule& shader_module);
    };

}