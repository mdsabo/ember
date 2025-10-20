#include "Window.h"

#include <SDL3/SDL_vulkan.h>

#include "ember/util/Log.h"

namespace ember::gpu {

    Window::Window(
        std::shared_ptr<const VulkanInstance> instance,
        const char* title,
        int w,
        int h,
        SDL_WindowFlags flags
    ): m_instance(instance->instance()) {
        m_window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_VULKAN);
        if (m_window == nullptr) {
            error(EMBER_GPU_LOG, "Failed to create window : {}", SDL_GetError());
            throw std::runtime_error("Failed to create window!");
        };

        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &surface)) {
            error(EMBER_GPU_LOG, "Failed to create surface : {}", SDL_GetError());
            throw std::runtime_error("Failed to create surface!");
        }
        m_surface = surface;
    }

    Window::~Window() {
        m_renderer = nullptr; // Clean this up before destroying the window
        SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
        SDL_DestroyWindow(m_window);
    }

    Window::Id Window::id() const {
        return SDL_GetWindowID(m_window);
    }

    std::pair<int, int> Window::size() const {
        int w, h;
        if (!SDL_GetWindowSize(m_window, &w, &h)) {
            error(EMBER_GPU_LOG, "Failed to get window[{}] size : {}", this->id(), SDL_GetError());
            throw std::runtime_error("Failed to get window size!");
        }
        return { w, h };
    }

    void Window::attach_renderer(std::shared_ptr<const GraphicsDevice> graphics_device) {
        m_renderer = std::make_unique<Renderer>(graphics_device);

        auto [ w, h ] = this->size();
        const vk::Extent2D window_extent {
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
        };
        m_swapchain = m_renderer->create_swapchain_for_surface(m_surface, window_extent);
    }

}