#pragma once

#include <array>

#include "ember/gpu/GraphicsInterface.h"
#include "Window.h"

namespace ember::graphics {

    class Renderer {
    public:
        Renderer(
            std::shared_ptr<const gpu::GraphicsDevice> gfx_device,
            std::unique_ptr<Window> window
        );
        ~Renderer();

        inline Window* window() { return m_window.get(); }
        inline const Window* window() const { return m_window.get(); }

        inline vk::Extent2D get_swapchain_extent() const {
            if (m_swapchain) return m_swapchain->extent;
            else return {};
        }

        vk::CommandBuffer begin_rendering();
        void end_rendering();
        void present_frame();

        inline gpu::GraphicsInterface* get_renderer() const { return m_gfx_interface.get(); }
        inline vk::Format get_swapchain_format() const {
            if (m_swapchain) return m_swapchain->format;
            else return vk::Format::eUndefined;
        }
        inline vk::CommandBuffer& get_active_command_buffer() {
            return m_per_frame_objects[m_frame_index].command_buffer;
        }

    private:
        static constexpr auto MAX_CONCURRENT_FRAMES = 3;

        std::shared_ptr<const gpu::GraphicsDevice> m_gfx_device;
        std::unique_ptr<gpu::GraphicsInterface> m_gfx_interface;

        std::unique_ptr<Window> m_window;
        gpu::Swapchain* m_swapchain;
        struct PerFrameObjects {
            vk::CommandBuffer command_buffer;
            vk::Fence wait_fence;
            vk::Semaphore present_complete_semaphore;
        };
        std::array<PerFrameObjects, MAX_CONCURRENT_FRAMES> m_per_frame_objects;
        size_t m_frame_index;
        uint32_t m_next_swapchain_image;
    };

}