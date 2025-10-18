#include "CommandRecorder.h"

namespace ember::gpu {

    CommandRecorder::CommandRecorder(CommandBuffer& command_buffer): m_command_buffer(command_buffer) { }

    void CommandRecorder::pipeline_barrier(
        vk::PipelineStageFlags src_stage_mask,
        vk::PipelineStageFlags dst_stage_mask,
        const ArrayProxy<vk::MemoryBarrier>& memory_barriers,
        const ArrayProxy<vk::BufferMemoryBarrier> buffer_memory_barriers,
        const ArrayProxy<vk::ImageMemoryBarrier> image_memory_barriers
    ) {
        m_command_buffer.pipelineBarrier(
            src_stage_mask,
            dst_stage_mask,
            {},
            memory_barriers,
            buffer_memory_barriers,
            image_memory_barriers
        );
    }

    void CommandRecorder::copy_buffer(const Buffer& dst, const Buffer& src, const std::vector<vk::BufferCopy>& copies) {
        if (copies.size() == 0) {
            m_command_buffer.copyBuffer(
                src.buffer,
                dst.buffer,
                vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = std::min(src.size, dst.size)}
            );
        } else {
            m_command_buffer.copyBuffer(src.buffer, dst.buffer, copies);
        }
    }

    void CommandRecorder::copy_image_to_buffer(const Buffer& buffer, const Image& image) {
        const vk::BufferImageCopy image_copy {
            .imageSubresource = vk::ImageSubresourceLayers{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageExtent = image.extent,
        };
        m_command_buffer.copyImageToBuffer(image.image, image.layout, buffer.buffer, image_copy);
    }

    void CommandRecorder::bind_pipeline(const Pipeline& pipeline) {
        m_command_buffer.bindPipeline(pipeline.bind_point, pipeline.pipeline);
    }

    void CommandRecorder::bind_descriptor_sets(
        const Pipeline& pipeline,
        uint32_t first_set,
        const DescriptorSetChunk& descriptor_sets
    ) {
        m_command_buffer.bindDescriptorSets(
            pipeline.bind_point,
            pipeline.layout,
            first_set,
            descriptor_sets.sets,
            {}
        );
    }

    void CommandRecorder::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
        m_command_buffer.dispatch(x, y, z);
    }

}