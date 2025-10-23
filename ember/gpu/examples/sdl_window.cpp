#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "ember/gpu/Renderer.h"
#include "ember/gpu/Window.h"
#include "ember/util/Log.h"
#include "Window.h"

using namespace ember::gpu;

struct Vertex {
    float pos[3];
    float color[3];
};

void run() {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());

    auto vkinstance = VulkanInstance::create({
        .features {
            .vk_layer_validation = true,
            //.vk_layer_api_dump = true
        }
    });
    auto window = Window(vkinstance, "SDL Playground", 800, 600);
    auto graphics_device = GraphicsDevice::create(vkinstance, window.surface());
    auto renderer = window.create_renderer(graphics_device);

    auto vertex_shader_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/sdl_window.vert");
    auto fragment_shader_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/sdl_window.frag");

    std::array shaders = {
        renderer->create_shader_module(vertex_shader_path),
        renderer->create_shader_module(fragment_shader_path)
    };

    auto descriptor_set_blueprints = renderer->create_descriptor_set_blueprints(shaders);

    const std::array<Renderer::ShaderStageInfo, 2> shader_stages {
        Renderer::ShaderStageInfo{ .module = shaders[0] },
        Renderer::ShaderStageInfo{ .module = shaders[1] }
    };
    const std::array vertex_bindings{
        vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = 2 * 3 * sizeof(float),
            .inputRate = vk::VertexInputRate::eVertex
        }
    };
    const vk::PipelineColorBlendAttachmentState color_blend_attachment {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA,
    };

    const auto swapchain_format = window.swapchain_format();

    // see https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    const Renderer::GraphicsPipelineState pipeline_state {
        .vertex_bindings = vertex_bindings,
        .input_assembly_state = vk::PipelineInputAssemblyStateCreateInfo {
            .topology = vk::PrimitiveTopology::eTriangleList,
        },
        .rasterization_state = vk::PipelineRasterizationStateCreateInfo {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable = vk::False,
            .lineWidth = 1.0,
        },
        .multisample_state = vk::PipelineMultisampleStateCreateInfo {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False,
        },
        .color_blend_state = vk::PipelineColorBlendStateCreateInfo {
            .logicOpEnable = vk::False,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment
        },
        .rendering_info = vk::PipelineRenderingCreateInfo {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain_format,
        }
    };
    auto pipeline = renderer->create_graphics_pipeline(
        shader_stages,
        pipeline_state,
        descriptor_set_blueprints
    );

    constexpr std::array VERTICES = {
        Vertex{ { -0.5, -0.5, 0.0 }, { 1.0, 0.0, 0.0 } },
        Vertex{ {  0.0,  0.5, 0.0 }, { 0.0, 1.0, 0.0 } },
        Vertex{ {  0.5, -0.5, 0.0 }, { 0.0, 0.0, 1.0 } }
    };
    auto vertex_buffer = renderer->create_vertex_buffer(
        sizeof(Vertex) * VERTICES.size(),
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    renderer->write_buffer<Vertex>(vertex_buffer, VERTICES);

    constexpr std::array<uint16_t, 3> INDICES = { 0, 1, 2 };
    auto index_buffer = renderer->create_index_buffer(
        sizeof(uint16_t) * INDICES.size(),
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    renderer->write_buffer<uint16_t>(index_buffer, INDICES);


    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }

        // should the image index be kept internal? probably right?
        auto image_index = window.begin_rendering_frame();
        auto swapchain_image = window.get_swapchain_image(image_index);

        auto command_buffer = renderer->create_command_buffer();
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
                    .image = swapchain_image->image,
                    .subresourceRange = vk::ImageSubresourceRange {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = vk::RemainingMipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = vk::RemainingArrayLayers
                    }
                }
            );

            // GET ME A HELPER FN FFS, yikers
            constexpr vk::ClearValue CLEAR_COLOR{.color = { .float32 = {{ 0.0f, 0.0f, 0.2f, 0.0f}} }};
            const vk::RenderingAttachmentInfo attachment_info {
                .imageView = swapchain_image->view,
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = CLEAR_COLOR
            };
            recorder.begin_rendering(swapchain_image, std::array{attachment_info});

            recorder.set_viewport(vk::Viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(swapchain_image->extent.width),
                .height = static_cast<float>(swapchain_image->extent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            });
            recorder.set_scissor(vk::Rect2D{
                .offset = { 0, 0 },
                .extent = { swapchain_image->extent.width, swapchain_image->extent.height }
            });

            recorder.bind_pipeline(pipeline);
            recorder.bind_vertex_buffer(vertex_buffer);
            recorder.bind_index_buffer(index_buffer, vk::IndexType::eUint16);
            recorder.draw_indexed(INDICES.size(), 1);

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
                    .image = swapchain_image->image,
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

        renderer->submit_command_buffer(
            command_buffer,
            window.frame_fence(),
            std::array{window.present_complete_semaphore()},
            std::array{
                vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            },
            std::array{window.get_swapchain_render_complete_semaphore(image_index)}
        );

        window.present_frame(image_index);
    }

    renderer->destroy_buffer(vertex_buffer);
    renderer->destroy_buffer(index_buffer);
    renderer->destroy_pipeline(pipeline);
    renderer->destroy_descriptor_set_blueprints(descriptor_set_blueprints);
    for (auto& shader : shaders) {
        renderer->destroy_shader_module(shader);
    }
}

int main(int argc, char* args[]) {
    run();
    return 0;
}