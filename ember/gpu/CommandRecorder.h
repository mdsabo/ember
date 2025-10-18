#pragma once

#include "RenderObjects.h"
#include "src/Vulkan.h"

namespace ember::gpu {

    class CommandRecorder {
    public:
        CommandRecorder(CommandBuffer& command_buffer, uint32_t queue_family_index);

        void pipeline_barrier(
            vk::PipelineStageFlags src_stage_mask,
            vk::PipelineStageFlags dst_stage_mask,
            const ArrayProxy<vk::MemoryBarrier>& memory_barriers,
            const ArrayProxy<vk::BufferMemoryBarrier> buffer_memory_barriers,
            const ArrayProxy<vk::ImageMemoryBarrier> image_memory_barriers
        );
        void transition_image_layout(Image& image, vk::ImageLayout new_layout);

        void copy_buffer(const Buffer& dst, const Buffer& src, const std::vector<vk::BufferCopy>& copies = {});
        void copy_image_to_buffer(const Buffer& buffer, const Image& image);

        void bind_pipeline(const Pipeline& pipeline);

        void bind_descriptor_sets(
            const Pipeline& pipeline,
            uint32_t first_set,
            const DescriptorSetChunk& descriptor_sets
        );

        void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

    private:
        CommandBuffer& m_command_buffer;
        uint32_t m_queue_family_index;
    };

}