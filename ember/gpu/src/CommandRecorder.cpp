#include "CommandRecorder.h"

namespace ember::gpu {

    CommandRecorder::CommandRecorder(vk::CommandBuffer command_buffer, uint32_t queue_family_index):
        m_command_buffer(command_buffer), m_queue_family_index(queue_family_index)
    { }

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

    namespace {
        constexpr vk::ImageSubresourceRange IMAGE_WHOLE_SUBRESOURCE_RANGE(vk::ImageAspectFlags aspect_mask) {
            return vk::ImageSubresourceRange {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = vk::RemainingMipLevels,
                .baseArrayLayer = 0,
                .layerCount = vk::RemainingArrayLayers
            };
        }
    }

    void CommandRecorder::transition_image_layout(Image* image, vk::ImageLayout new_layout) {
        const vk::ImageMemoryBarrier image_memory_barrier {
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
            .oldLayout = image->layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = m_queue_family_index,
            .dstQueueFamilyIndex = m_queue_family_index,
            .image = image->image,
            .subresourceRange = IMAGE_WHOLE_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor)
        };

        pipeline_barrier(
            vk::PipelineStageFlagBits::eTopOfPipe, // TODO: Could expose these out as a slight optimization?
            vk::PipelineStageFlagBits::eTransfer,
            {},
            {},
            image_memory_barrier
        );
        image->layout = new_layout;
    }

    void CommandRecorder::fill_buffer(Buffer* dst, uint32_t value, vk::DeviceSize offset, vk::DeviceSize size) {
        m_command_buffer.fillBuffer(dst->buffer, offset, size, value);
    }

    void CommandRecorder::copy_buffer(Buffer* dst, const Buffer* src, const std::vector<vk::BufferCopy>& copies) {
        if (copies.size() == 0) {
            m_command_buffer.copyBuffer(
                src->buffer,
                dst->buffer,
                vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = std::min(src->size, dst->size)}
            );
        } else {
            m_command_buffer.copyBuffer(src->buffer, dst->buffer, copies);
        }
    }

    void CommandRecorder::copy_image_to_buffer(Buffer* buffer, const Image* image) {
        const vk::BufferImageCopy image_copy {
            .imageSubresource = vk::ImageSubresourceLayers{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageExtent = image->extent,
        };
        m_command_buffer.copyImageToBuffer(image->image, image->layout, buffer->buffer, image_copy);
    }

    void CommandRecorder::bind_vertex_buffer(
        const Buffer* buffer,
        uint32_t binding_index,
        vk::DeviceSize offset
    ) {
        m_command_buffer.bindVertexBuffers(binding_index, buffer->buffer, offset);
    }

    void CommandRecorder::bind_index_buffer(
        const Buffer* buffer,
        vk::IndexType index_type,
        vk::DeviceSize offset
    ) {
        m_command_buffer.bindIndexBuffer(buffer->buffer, offset, index_type);
    }

    void CommandRecorder::bind_pipeline(const Pipeline* pipeline) {
        m_command_buffer.bindPipeline(pipeline->bind_point, pipeline->pipeline);
    }

    void CommandRecorder::bind_descriptor_sets(
        const Pipeline* pipeline,
        uint32_t first_set,
        const ArrayProxy<vk::DescriptorSet>& descriptor_sets
    ) {
        m_command_buffer.bindDescriptorSets(
            pipeline->bind_point,
            pipeline->layout,
            first_set,
            descriptor_sets,
            {}
        );
    }

    void CommandRecorder::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
        m_command_buffer.dispatch(x, y, z);
    }

    void CommandRecorder::begin_rendering(
        Image* image,
        const std::span<const vk::RenderingAttachmentInfo>& color_attachments,
        const vk::RenderingAttachmentInfo* depth_attachment,
        const vk::RenderingAttachmentInfo* stencil_attachment
    ) {
        const vk::RenderingInfo rendering_info {
            .renderArea = { 0, 0, image->extent.width, image->extent.height },
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(color_attachments.size()),
            .pColorAttachments = color_attachments.data(),
            .pDepthAttachment = depth_attachment,
            .pStencilAttachment = stencil_attachment
        };
        m_command_buffer.beginRendering(rendering_info);
    }

    void CommandRecorder::end_rendering() {
        m_command_buffer.endRendering();
    }

    void CommandRecorder::set_viewport(const vk::Viewport& viewport) {
        m_command_buffer.setViewport(0, viewport);
    }
    void CommandRecorder::set_scissor(const vk::Rect2D& scissor) {
        m_command_buffer.setScissor(0, scissor);
    }

    void CommandRecorder::draw_indexed(
        uint32_t index_count,
        uint32_t instance_count,
        uint32_t first_index,
        int32_t vertex_offset,
        uint32_t first_instance
    ) {
        m_command_buffer.drawIndexed(
            index_count,
            instance_count,
            first_index,
            vertex_offset,
            first_instance
        );
    }

}