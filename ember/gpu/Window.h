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
        inline vk::Format swapchain_format() const {
            if (m_swapchain) return m_swapchain->format;
            else return vk::Format::eUndefined;
        }
        inline vk::Extent2D swapchain_extent() const {
            if (m_swapchain) return m_swapchain->extent;
            else return {};
        }

        Renderer* create_renderer(std::shared_ptr<const GraphicsDevice> graphics_device);
        Renderer* get_renderer() { return m_renderer.get(); }

        uint32_t begin_rendering_frame();
        inline Image* get_swapchain_image(uint32_t image_index) {
            return m_swapchain->images.at(image_index).first;
        }
        inline vk::Semaphore get_swapchain_render_complete_semaphore(uint32_t image_index) {
            return m_swapchain->images.at(image_index).second;
        }
        inline vk::Fence frame_fence() {
            return m_frame_sync_objects[m_frame_index].wait_fence;
        }
        inline vk::Semaphore present_complete_semaphore() {
            return m_frame_sync_objects[m_frame_index].present_complete;
        }
        void present_frame(uint32_t image_index);

    private:
        static constexpr auto MAX_CONCURRENT_FRAMES = 3;

        vk::Instance m_instance;
        SDL_Window* m_window;
        vk::SurfaceKHR m_surface;

        std::unique_ptr<Renderer> m_renderer;
        Swapchain* m_swapchain;
        struct FrameSyncObjects {
            vk::Fence wait_fence;
            vk::Semaphore present_complete;
        };
        std::array<FrameSyncObjects, MAX_CONCURRENT_FRAMES> m_frame_sync_objects;
        size_t m_frame_index;
    };

}