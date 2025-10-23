#pragma once

#include <vulkan/vulkan.hpp>
#include "RenderObjects.h"

namespace ember::gpu {

    using vk::ArrayProxy;

    class CommandRecorder {
    public:
        CommandRecorder(vk::CommandBuffer command_buffer, uint32_t queue_family_index);

        void pipeline_barrier(
            vk::PipelineStageFlags src_stage_mask,
            vk::PipelineStageFlags dst_stage_mask,
            const ArrayProxy<vk::MemoryBarrier>& memory_barriers,
            const ArrayProxy<vk::BufferMemoryBarrier> buffer_memory_barriers,
            const ArrayProxy<vk::ImageMemoryBarrier> image_memory_barriers
        );
        void transition_image_layout(Image* image, vk::ImageLayout new_layout);

        void fill_buffer(Buffer* dst, uint32_t value, vk::DeviceSize offset, vk::DeviceSize size);
        void copy_buffer(Buffer* dst, const Buffer* src, const std::vector<vk::BufferCopy>& copies = {});
        void copy_image_to_buffer(Buffer* buffer, const Image* image);

        void bind_vertex_buffer(
            const Buffer* buffer,
            uint32_t binding_index = 0,
            vk::DeviceSize offset = 0
        );
        void bind_index_buffer(
            const Buffer* buffer,
            vk::IndexType index_type,
            vk::DeviceSize offset = 0
        );

        void bind_pipeline(const Pipeline* pipeline);

        void bind_descriptor_sets(
            const Pipeline* pipeline,
            uint32_t first_set,
            const ArrayProxy<vk::DescriptorSet>& descriptor_sets
        );

        void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

        void begin_rendering(
            Image* swapchain,
            const std::span<const vk::RenderingAttachmentInfo>& color_attachments,
            const vk::RenderingAttachmentInfo* depth_attachment = nullptr,
            const vk::RenderingAttachmentInfo* stencil_attachment = nullptr
        );
        void end_rendering();

        void set_viewport(const vk::Viewport& viewport);
        void set_scissor(const vk::Rect2D& scissor);

        void draw_indexed(
            uint32_t index_count,
            uint32_t instance_count,
            uint32_t first_index = 0,
            int32_t vertex_offset = 0,
            uint32_t first_instance = 0
        );

    private:
        vk::CommandBuffer m_command_buffer;
        uint32_t m_queue_family_index;
    };

}