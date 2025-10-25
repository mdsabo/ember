#include "Window.h"

#include <SDL3/SDL_vulkan.h>

#include "ember/gpu/VulkanHelpers.h"
#include "ember/util/Log.h"

namespace ember::graphics {
    using namespace gpu;

    Window::Window(
        std::shared_ptr<const VulkanInstance> instance,
        const char* title,
        int w,
        int h,
        SDL_WindowFlags flags
    ): m_instance(instance) {
        m_window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_VULKAN);
        if (m_window == nullptr) {
            error(EMBER_GRAPHICS_LOG, "Failed to create window : {}", SDL_GetError());
            throw std::runtime_error("Failed to create window!");
        };

        m_surface = m_instance->create_window_surface(m_window);
    }

    void Window::destroy_render_objects() {
        std::array<vk::Fence, MAX_CONCURRENT_FRAMES> fences;
        for (auto i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
            fences[i] = m_per_frame_objects[i].wait_fence;
        }
        m_renderer->wait_for_fences(fences);

        for (auto& fobj : m_per_frame_objects) {
            m_renderer->destroy_command_buffer(fobj.command_buffer);
            m_renderer->destroy_fence(fobj.wait_fence);
            m_renderer->destroy_semaphore(fobj.present_complete_semaphore);
        }

        m_renderer->destroy_swapchain(m_swapchain);
        m_renderer = nullptr;
    }

    Window::~Window() {
        if (m_renderer) destroy_render_objects();
        if (m_window) {
            m_instance->destroy_window_surface(m_surface);
            SDL_DestroyWindow(m_window);
        }
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
                const CommandRecorder::ImageTransitionInfo info {
                    .new_layout = vk::ImageLayout::eColorAttachmentOptimal,
                    .src_pipeline_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    .dst_pipeline_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    .dst_access_mask = vk::AccessFlagBits::eColorAttachmentWrite,
                    .subresource_range = MAX_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor)
                };
                recorder.transition_image_layout(image, info);

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

        m_next_swapchain_image = m_renderer->get_next_swapchain_image(
            m_swapchain,
            fobj.present_complete_semaphore
        );

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

                const CommandRecorder::ImageTransitionInfo info {
                    .new_layout = vk::ImageLayout::ePresentSrcKHR,
                    .src_pipeline_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    .dst_pipeline_stage = vk::PipelineStageFlagBits::eNoneKHR,
                    .src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite,
                    .subresource_range = MAX_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor)
                };
                recorder.transition_image_layout(image, info);
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