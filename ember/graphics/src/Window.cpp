#include "Window.h"

#include <SDL3/SDL_vulkan.h>

#include "ember/gpu/VulkanHelpers.h"
#include "ember/util/Log.h"

namespace ember::graphics {
    using namespace gpu;

    Window::Window(
        std::shared_ptr<const VulkanInstance> instance,
        const WindowSettings& settings
    ): m_instance(instance) {
        m_window = SDL_CreateWindow(settings.title, settings.width, settings.height, settings.flags | SDL_WINDOW_VULKAN);
        if (m_window == nullptr) {
            error(EMBER_GRAPHICS_LOG, "Failed to create window : {}", SDL_GetError());
            throw std::runtime_error("Failed to create window!");
        };

        m_surface = m_instance->create_window_surface(m_window);
    }

    // void Window::destroy_render_objects() {
    //     std::array<vk::Fence, MAX_CONCURRENT_FRAMES> fences;
    //     for (auto i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
    //         fences[i] = m_per_frame_objects[i].wait_fence;
    //     }
    //     m_renderer->wait_for_fences(fences);

    //     for (auto& fobj : m_per_frame_objects) {
    //         m_renderer->destroy_command_buffer(fobj.command_buffer);
    //         m_renderer->destroy_fence(fobj.wait_fence);
    //         m_renderer->destroy_semaphore(fobj.present_complete_semaphore);
    //     }

    //     m_renderer->destroy_swapchain(m_swapchain);
    //     m_renderer = nullptr;
    // }

    Window::~Window() {
        m_instance->destroy_window_surface(m_surface);
        SDL_DestroyWindow(m_window);
    }

    Window::Id Window::id() const {
        return SDL_GetWindowID(m_window);
    }

    std::pair<int, int> Window::size() const {
        int w, h;
        if (!SDL_GetWindowSize(m_window, &w, &h)) {
            error(EMBER_GRAPHICS_LOG, "Failed to get window[{}] size : {}", this->id(), SDL_GetError());
            throw std::runtime_error("Failed to get window size!");
        }
        return { w, h };
    }

    void Window::set_visible(bool visible) {
        bool res;
        if (visible) res = SDL_ShowWindow(m_window);
        else res = SDL_HideWindow(m_window);
        if (!res) {
            error(EMBER_GRAPHICS_LOG, "Failed to set window visibility ({}): {}", visible, SDL_GetError());
            throw std::runtime_error("Failed to set window visibility");
        }
    }

    void Window::set_fullscreen(bool fullscreen) {
        bool res = SDL_SetWindowFullscreen(m_window, fullscreen);
        if (!res) {
            error(EMBER_GRAPHICS_LOG, "Failed to set window fullscreen to {}: {}", fullscreen, SDL_GetError());
            throw std::runtime_error("Failed to set window fullscreen");
        }
    }

}