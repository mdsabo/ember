#pragma once

#include <array>

#include "Window.h"
#include "ember/gpu/GPUInterface.h"

namespace ember::graphics {

    class Renderpass {
    public:
        static constexpr auto MAX_CONCURRENT_FRAMES = 3;

        Renderpass(gpu::GPUInterface* gpu_interface);
        virtual ~Renderpass() = default;

        struct RenderpassObjects {
            gpu::Pipeline* pipeline;
            vk::CommandBuffer command_buffer;
        };
        virtual RenderpassObjects begin(gpu::GPUInterface* gpu_interface) = 0;
        virtual void end(gpu::GPUInterface* gpu_interface) = 0;
        virtual void destroy(gpu::GPUInterface* gpu_interface) = 0;

    protected:
        struct PerFrameObjects {
            vk::CommandBuffer command_buffer;
            vk::Fence wait_fence;
            vk::Semaphore present_complete_semaphore;
        };
        std::array<PerFrameObjects, MAX_CONCURRENT_FRAMES> m_per_frame_objects;
        size_t m_frame_index;

        inline void increment_frame_index() {
            m_frame_index = (m_frame_index + 1) % MAX_CONCURRENT_FRAMES;
        }
        inline PerFrameObjects& current_frame_objs() {
            return m_per_frame_objects.at(m_frame_index);
        }

        void destroy_per_frame_objects(gpu::GPUInterface* gpu_interface);
    };

    class SwapchainRenderpass : public Renderpass {
    public:
        SwapchainRenderpass(gpu::GPUInterface* gpu_interface, Window* window);
        virtual ~SwapchainRenderpass() = default;

        virtual RenderpassObjects begin(gpu::GPUInterface* gpu_interface) override;
        virtual void end(gpu::GPUInterface* gpu_interface) override;
        virtual void destroy(gpu::GPUInterface* gpu_interface) override;

        inline vk::Extent2D get_swapchain_extent() const {
            if (m_swapchain) return m_swapchain->extent;
            else return {};
        }
    private:
        gpu::Swapchain* m_swapchain;
        uint32_t m_next_swapchain_image;
    };
}