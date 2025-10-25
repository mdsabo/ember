#pragma once

#include <vulkan/vulkan.hpp>
#include "RenderObjects.h"

namespace ember::gpu {

    class CommandRecorder {
    public:
        CommandRecorder(vk::CommandBuffer command_buffer, uint32_t queue_family_index);

        struct ImageTransitionInfo {
            vk::ImageLayout new_layout;
            vk::PipelineStageFlags src_pipeline_stage;
            vk::PipelineStageFlags dst_pipeline_stage;
            vk::AccessFlags src_access_mask;
            vk::AccessFlags dst_access_mask;
            vk::ImageSubresourceRange subresource_range;
        };
        void transition_image_layout(Image* image, const ImageTransitionInfo& info);

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
            vk::IndexType index_type = vk::IndexType::eUint32,
            vk::DeviceSize offset = 0
        );

        void bind_pipeline(const Pipeline* pipeline);

        void bind_descriptor_sets(
            const Pipeline* pipeline,
            uint32_t first_set,
            const std::span<const vk::DescriptorSet>& descriptor_sets
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

        void draw(
            uint32_t vertex_count,
            uint32_t instance_count,
            uint32_t first_vertex = 0,
            uint32_t first_instance = 0
        );
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