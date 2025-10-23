#include "Window.h"

#include <SDL3/SDL_vulkan.h>

#include "VulkanHelpers.h"
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
            for (auto& fobj : m_per_frame_objects) {
                m_renderer->destroy_command_buffer(fobj.command_buffer);
                m_renderer->destroy_fence(fobj.wait_fence);
                m_renderer->destroy_semaphore(fobj.present_complete_semaphore);
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

        for (auto& obj : m_per_frame_objects) {
            obj.command_buffer = m_renderer->create_command_buffer();
            obj.wait_fence = m_renderer->create_fence(true);
            obj.present_complete_semaphore = m_renderer->create_semaphore();
        }
        m_frame_index = MAX_CONCURRENT_FRAMES - 1; // start at "last" frame so we wrap to 0 on first render

        return m_renderer.get();
    }

    namespace {
        void record_begin_rendering_commands(
            Renderer* renderer,
            vk::CommandBuffer command_buffer,
            Image* image
        ) {
            renderer->record_command_buffer(command_buffer, [&](CommandRecorder& recorder) {
                recorder.pipeline_barrier(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    {},
                    {},
                    vk::ImageMemoryBarrier{
                        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                        .oldLayout = vk::ImageLayout::eUndefined,
                        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                        .srcQueueFamilyIndex = 0,
                        .dstQueueFamilyIndex = 0,
                        .image = image->image,
                        .subresourceRange = vk::ImageSubresourceRange {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .baseMipLevel = 0,
                            .levelCount = vk::RemainingMipLevels,
                            .baseArrayLayer = 0,
                            .layerCount = vk::RemainingArrayLayers
                        }
                    }
                );

                const vk::RenderingAttachmentInfo attachment_info {
                    .imageView = image->view,
                    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                    .loadOp = vk::AttachmentLoadOp::eClear,
                    .storeOp = vk::AttachmentStoreOp::eStore,
                    .clearValue = CLEAR_COLOR(0.0f, 0.0f, 0.2f, 0.0f),
                };
                recorder.begin_rendering(image, std::array{attachment_info});

                recorder.set_viewport(vk::Viewport{
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(image->extent.width),
                    .height = static_cast<float>(image->extent.height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f
                });
                recorder.set_scissor(vk::Rect2D{
                    .offset = { 0, 0 },
                    .extent = { image->extent.width, image->extent.height }
                });
            });
        }
    }

    vk::CommandBuffer& Window::begin_rendering_frame() {
        m_frame_index = (m_frame_index + 1) % MAX_CONCURRENT_FRAMES;

        auto& fobj = m_per_frame_objects.at(m_frame_index);

        const std::array fence{fobj.wait_fence};
        m_renderer->wait_for_fences(fence);
        m_renderer->reset_fences(fence);

        m_next_swapchain_image = m_renderer->get_next_swapchain_image(m_swapchain, fobj.present_complete_semaphore);

        m_renderer->restart_command_buffer(fobj.command_buffer);

        record_begin_rendering_commands(
            m_renderer.get(),
            fobj.command_buffer,
            m_swapchain->images.at(m_next_swapchain_image).first
        );

        return fobj.command_buffer;
    }

    namespace {
        void record_end_rendering_commands(
            Renderer* renderer,
            vk::CommandBuffer command_buffer,
            Image* image
        ) {
            renderer->record_command_buffer(command_buffer, [&](CommandRecorder& recorder) {
                recorder.end_rendering();

                //recorder.transition_image_layout(swapchain_image, vk::ImageLayout::ePresentSrcKHR);
                recorder.pipeline_barrier(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    {},
                    {},
                    vk::ImageMemoryBarrier{
                        .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                        .newLayout = vk::ImageLayout::ePresentSrcKHR,
                        .srcQueueFamilyIndex = 0,
                        .dstQueueFamilyIndex = 0,
                        .image = image->image,
                        .subresourceRange = vk::ImageSubresourceRange {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .baseMipLevel = 0,
                            .levelCount = vk::RemainingMipLevels,
                            .baseArrayLayer = 0,
                            .layerCount = vk::RemainingArrayLayers
                        }
                    }
                );
            });
        }
    }

    void Window::present_frame() {
        auto& fobj = m_per_frame_objects.at(m_frame_index);

        record_end_rendering_commands(
            m_renderer.get(),
            fobj.command_buffer,
            m_swapchain->images.at(m_next_swapchain_image).first
        );

        m_renderer->submit_command_buffer(
            fobj.command_buffer,
            fobj.wait_fence,
            std::array{fobj.present_complete_semaphore},
            std::array{
                vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            },
            std::array{m_swapchain->images.at(m_next_swapchain_image).second}
        );

        m_renderer->present_swapchain(m_swapchain, m_next_swapchain_image);
    }

}