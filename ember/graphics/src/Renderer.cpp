#include "Renderer.h"

#include "ember/gpu/VulkanHelpers.h"

namespace ember::graphics {

    Renderer::Renderer(
        std::shared_ptr<const gpu::GraphicsDevice> gfx_device,
        std::unique_ptr<Window> window
    ): m_gfx_device(gfx_device), m_window(std::move(window)), m_frame_index(MAX_CONCURRENT_FRAMES) {
        m_gfx_interface = std::make_unique<gpu::GraphicsInterface>(m_gfx_device);

        auto [ w, h ] = m_window->size();
        const vk::Extent2D window_extent {
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
        };
        m_swapchain = m_gfx_interface->create_swapchain_for_surface(m_window->surface(), window_extent);

        for (auto& obj : m_per_frame_objects) {
            obj.command_buffer = m_gfx_interface->create_command_buffer();
            obj.wait_fence = m_gfx_interface->create_fence(true);
            obj.present_complete_semaphore = m_gfx_interface->create_semaphore();
        }
    }

    Renderer::~Renderer() {
        std::array<vk::Fence, MAX_CONCURRENT_FRAMES> fences;
        for (auto i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
            fences[i] = m_per_frame_objects[i].wait_fence;
        }
        m_gfx_interface->wait_for_fences(fences);

        for (auto& fobj : m_per_frame_objects) {
            m_gfx_interface->destroy_command_buffer(fobj.command_buffer);
            m_gfx_interface->destroy_fence(fobj.wait_fence);
            m_gfx_interface->destroy_semaphore(fobj.present_complete_semaphore);
        }

        m_gfx_interface->destroy_swapchain(m_swapchain);
        m_gfx_interface = nullptr;
        m_window = nullptr;
    }

    vk::CommandBuffer Renderer::begin_rendering() {
        m_frame_index = (m_frame_index + 1) % MAX_CONCURRENT_FRAMES;

        auto& fobj = m_per_frame_objects.at(m_frame_index);

        const std::array fence{fobj.wait_fence};
        m_gfx_interface->wait_for_fences(fence);
        m_gfx_interface->reset_fences(fence);

        m_next_swapchain_image = m_gfx_interface->get_next_swapchain_image(
            m_swapchain,
            fobj.present_complete_semaphore
        );

        m_gfx_interface->restart_command_buffer(fobj.command_buffer);
        m_gfx_interface->begin_rendering_to_swapchain(
            m_swapchain,
            m_next_swapchain_image,
            fobj.command_buffer,
            gpu::CLEAR_COLOR(0.0, 0.0, 0.0, 0.0)
        );

        const auto& render_target = m_swapchain->render_targets.at(m_next_swapchain_image);
        m_gfx_interface->record_command_buffer(fobj.command_buffer, [&](gpu::CommandRecorder& recorder) {
            // FIXME
            recorder.set_scissor(vk::Rect2D{
                .offset = { 0, 0 },
                .extent = { render_target.image->extent.width, render_target.image->extent.height }
            });
        });

        return fobj.command_buffer;
    }

    void Renderer::end_rendering() {
        auto& fobj = m_per_frame_objects.at(m_frame_index);
        auto& render_target = m_swapchain->render_targets.at(m_next_swapchain_image);

        m_gfx_interface->end_rendering_to_swapchain(m_swapchain, m_next_swapchain_image, fobj.command_buffer);

        m_gfx_interface->submit_command_buffer(
            fobj.command_buffer,
            fobj.wait_fence,
            std::array{fobj.present_complete_semaphore},
            std::array{
                vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            },
            std::array{render_target.render_complete}
        );
    }

    void Renderer::present_frame() {
        m_gfx_interface->present_swapchain(m_swapchain, m_next_swapchain_image);
    }

}