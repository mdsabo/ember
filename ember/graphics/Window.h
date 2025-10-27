#pragma once

#include <array>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

#include "ember/gpu/VulkanInstance.h"
#include "ember/gpu/GPUDevice.h"
#include "ember/gpu/GPUInterface.h"

namespace ember::graphics {

    struct WindowSettings {
        const char* title;
        int width;
        int height;
        SDL_WindowFlags flags;
    };

    class Window {
    public:
        using Id = SDL_WindowID;

        Window() = default;
        Window(
            std::shared_ptr<const gpu::VulkanInstance> instance,
            const WindowSettings& settings
        );
        ~Window();

        Id id() const;
        std::pair<int, int> size() const;
        inline vk::SurfaceKHR surface() const { return m_surface; }
        // inline vk::Format get_swapchain_format() const {
        //     if (m_swapchain) return m_swapchain->format;
        //     else return vk::Format::eUndefined;
        // }
        // inline vk::Extent2D get_swapchain_extent() const {
        //     if (m_swapchain) return m_swapchain->extent;
        //     else return {};
        // }

        void set_visible(bool visible);
        void set_fullscreen(bool fullscreen);

        // gpu::GraphicsInterface* create_renderer(std::shared_ptr<const gpu::GPUDevice> graphics_device);
        // gpu::GraphicsInterface* get_renderer() { return m_renderer.get(); }

        // vk::CommandBuffer& begin_rendering_frame();
        // inline vk::CommandBuffer& get_active_command_buffer() {
        //     return m_per_frame_objects[m_frame_index].command_buffer;
        // }
        // void present_frame();

    private:
        // static constexpr auto MAX_CONCURRENT_FRAMES = 3;

        std::shared_ptr<const gpu::VulkanInstance> m_instance;
        SDL_Window* m_window;
        vk::SurfaceKHR m_surface;

        // std::unique_ptr<gpu::GraphicsInterface> m_renderer;
        // gpu::Swapchain* m_swapchain;
        // struct PerFrameObjects {
        //     vk::CommandBuffer command_buffer;
        //     vk::Fence wait_fence;
        //     vk::Semaphore present_complete_semaphore;
        // };
        // std::array<PerFrameObjects, MAX_CONCURRENT_FRAMES> m_per_frame_objects;
        // size_t m_frame_index;
        // uint32_t m_next_swapchain_image;

        // void destroy_render_objects();
    };

}