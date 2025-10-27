#include "Renderpass.h"

#include "ember/gpu/VulkanHelpers.h"

namespace ember::graphics {

    Renderpass::Renderpass(gpu::GPUInterface* gpu_interface) {
        for (auto& obj : m_per_frame_objects) {
            obj.command_buffer = gpu_interface->create_command_buffer();
            obj.wait_fence = gpu_interface->create_fence(true);
            obj.present_complete_semaphore = gpu_interface->create_semaphore();
        }
    }

    void Renderpass::destroy_per_frame_objects(gpu::GPUInterface* gpu_interface) {
        for (auto& fobj : m_per_frame_objects) {
            gpu_interface->destroy_command_buffer(fobj.command_buffer);
            gpu_interface->destroy_fence(fobj.wait_fence);
            gpu_interface->destroy_semaphore(fobj.present_complete_semaphore);
        }
    }

    SwapchainRenderpass::SwapchainRenderpass(gpu::GPUInterface* gpu_interface, Window* window): Renderpass(gpu_interface) {
        auto [ w, h ] = window->size();
        const vk::Extent2D window_extent {
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
        };
        m_swapchain = gpu_interface->create_swapchain_for_surface(window->surface(), window_extent);
    }

    Renderpass::RenderpassObjects SwapchainRenderpass::begin(gpu::GPUInterface* gpu_interface) {
        increment_frame_index();

        auto& fobj = current_frame_objs();

        const std::array fence{fobj.wait_fence};
        gpu_interface->wait_for_fences(fence);
        gpu_interface->reset_fences(fence);

        m_next_swapchain_image = gpu_interface->get_next_swapchain_image(
            m_swapchain,
            fobj.present_complete_semaphore
        );

        gpu_interface->restart_command_buffer(fobj.command_buffer);
        gpu_interface->begin_rendering_to_swapchain(
            m_swapchain,
            m_next_swapchain_image,
            fobj.command_buffer,
            gpu::CLEAR_COLOR(0.0, 0.0, 0.0, 0.0)
        );

        return { nullptr, fobj.command_buffer };
    }

    void SwapchainRenderpass::end(gpu::GPUInterface* gpu_interface) {
        auto& fobj = m_per_frame_objects.at(m_frame_index);
        auto& render_target = m_swapchain->render_targets.at(m_next_swapchain_image);

        gpu_interface->end_rendering_to_swapchain(m_swapchain, m_next_swapchain_image, fobj.command_buffer);

        gpu_interface->submit_command_buffer(
            fobj.command_buffer,
            fobj.wait_fence,
            std::array{fobj.present_complete_semaphore},
            std::array{
                vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            },
            std::array{render_target.render_complete}
        );

        gpu_interface->present_swapchain(m_swapchain, m_next_swapchain_image);
    }

    void SwapchainRenderpass::destroy(gpu::GPUInterface* gpu_interface) {
        destroy_per_frame_objects(gpu_interface);
        gpu_interface->destroy_swapchain(m_swapchain);
    }

}