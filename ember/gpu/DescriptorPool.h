#pragma once

#include <vector>

#include "Vulkan.h"

namespace ember::gpu {

    static constexpr uint32_t DEFAULT_DESCRIPTOR_POOL_SIZE = 1024;

    struct DescriptorSetCounts {
        uint32_t samplers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_SAMPLER = 0, */
        uint32_t combined_image_samplers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1, */
        uint32_t sampled_images = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2, */
        uint32_t storage_images = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3, */
        uint32_t uniform_texel_buffers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4, */
        uint32_t storage_texel_buffers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5, */
        uint32_t uniform_buffers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6, */
        uint32_t storage_buffers = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7, */
        uint32_t uniform_buffer_dynamics = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8, */
        uint32_t storage_buffer_dynamics = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9, */
        uint32_t input_attachments = DEFAULT_DESCRIPTOR_POOL_SIZE; /* VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10, */
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

    class DescriptorPool {
    public:
        DescriptorPool() = default;
        DescriptorPool(vk::DescriptorPool pool, DescriptorSetCounts max_sets);
        DescriptorPool(const DescriptorPool&) = default;

        void allocate_descriptor_set(vk::DescriptorType type);
        void free_descriptor_set(vk::DescriptorType type);

        inline vk::DescriptorPool pool() { return m_pool; }
    private:
        vk::DescriptorPool m_pool;
        DescriptorSetCounts m_allocated;            ///< Currently allocated set count
        DescriptorSetCounts m_max_allocated;  ///< Max allowed allocated sets
        DescriptorSetCounts m_higherwater;          ///< Highwater mark of set allocation count
    };

}