#pragma once

#include <array>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

#include "VulkanInstance.h"
#include "GraphicsDevice.h"
#include "Renderer.h"

namespace ember::gpu {

    class Window {
    public:
        using Id = SDL_WindowID;

        Window(
            std::shared_ptr<const VulkanInstance> instance,
            const char* title,
            int w,
            int h,
            SDL_WindowFlags flags = 0
        );
        ~Window();

        Id id() const;
        std::pair<int, int> size() const;
        inline vk::SurfaceKHR surface() const { return m_surface; }
        inline vk::Format get_swapchain_format() const {
            if (m_swapchain) return m_swapchain->format;
            else return vk::Format::eUndefined;
        }
        inline vk::Extent2D get_swapchain_extent() const {
            if (m_swapchain) return m_swapchain->extent;
            else return {};
        }

        Renderer* create_renderer(std::shared_ptr<const GraphicsDevice> graphics_device);
        Renderer* get_renderer() { return m_renderer.get(); }

        vk::CommandBuffer& begin_rendering_frame();
        void present_frame();

    private:
        static constexpr auto MAX_CONCURRENT_FRAMES = 3;

        vk::Instance m_instance;
        SDL_Window* m_window;
        vk::SurfaceKHR m_surface;

        std::unique_ptr<Renderer> m_renderer;
        Swapchain* m_swapchain;
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