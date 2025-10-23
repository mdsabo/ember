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
        if (m_renderer) {
            for (auto& sync : m_frame_sync_objects) {
                m_renderer->destroy_fence(sync.wait_fence);
                m_renderer->destroy_semaphore(sync.present_complete);
            }

            m_renderer->destroy_swapchain(m_swapchain);
            m_renderer = nullptr; // Clean this up before destroying the window
        }
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

    Renderer* Window::create_renderer(std::shared_ptr<const GraphicsDevice> graphics_device) {
        m_renderer = std::make_unique<Renderer>(graphics_device);

        auto [ w, h ] = this->size();
        const vk::Extent2D window_extent {
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
        };
        m_swapchain = m_renderer->create_swapchain_for_surface(m_surface, window_extent);

        for (auto& obj : m_frame_sync_objects) {
            obj.wait_fence = m_renderer->create_fence(true),
            obj.present_complete = m_renderer->create_semaphore();
        }
        m_frame_index = MAX_CONCURRENT_FRAMES - 1; // start at "last" frame so we wrap to 0 on first render

        return m_renderer.get();
    }

    uint32_t Window::begin_rendering_frame() {
        m_frame_index = (m_frame_index + 1) % MAX_CONCURRENT_FRAMES;

        auto& sync = m_frame_sync_objects.at(m_frame_index);

        const std::array fence{sync.wait_fence};
        m_renderer->wait_for_fences(fence);
        m_renderer->reset_fences(fence);

        return m_renderer->get_next_swapchain_image(m_swapchain, sync.present_complete);
    }

    void Window::present_frame(uint32_t image_index) {
        m_renderer->present_swapchain(m_swapchain, image_index);
    }

}