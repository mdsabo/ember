#pragma once

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

        Renderer* create_renderer(std::shared_ptr<const GraphicsDevice> graphics_device);
        Renderer* get_renderer() { return m_renderer.get(); }

    private:
        vk::Instance m_instance;
        SDL_Window* m_window;
        vk::SurfaceKHR m_surface;

        std::unique_ptr<Renderer> m_renderer;
        vk::SwapchainKHR m_swapchain;
    };

}