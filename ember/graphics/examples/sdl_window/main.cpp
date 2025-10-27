#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "ember/gpu/GPUInterface.h"
#include "ember/gpu/VulkanHelpers.h"
#include "ember/util/Log.h"
#include "Window.h"

using namespace ember::gpu;
using namespace ember::graphics;

struct Vertex {
    float pos[3];
    float color[3];
};

void run() {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());

    auto vkinstance = VulkanInstance::create({
        .features {
            .vk_layer_validation = true,
            // .vk_layer_api_dump = true
        }
    });
    auto window = Window(vkinstance, "SDL Playground", 800, 600);
    auto graphics_device = GPUDevice::create(vkinstance, window.surface());
    auto gfxinterface = window.create_renderer(graphics_device);

    auto vertex_shader_path = std::filesystem::path(EMBER_GRAPHICS_DIR)
        .append("examples/sdl_window/sdl_window.vert");
    auto fragment_shader_path = std::filesystem::path(EMBER_GRAPHICS_DIR)
        .append("examples/sdl_window/sdl_window.frag");

    std::array shaders = {
        gfxinterface->create_shader_module(vertex_shader_path),
        gfxinterface->create_shader_module(fragment_shader_path)
    };

    auto descriptor_set_blueprints = gfxinterface->create_descriptor_set_blueprints(shaders);

    const std::array<GPUInterface::ShaderStageInfo, 2> shader_stages {
        GPUInterface::ShaderStageInfo{ .module = shaders[0] },
        GPUInterface::ShaderStageInfo{ .module = shaders[1] }
    };
    const std::array vertex_bindings{
        vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
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

    const auto swapchain_format = window.get_swapchain_format();

    // see https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
    const GPUInterface::GraphicsPipelineState pipeline_state {
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
        // FIXME get some of this stuff from the window
        .rendering_info = vk::PipelineRenderingCreateInfo {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain_format,
        }
    };
    auto pipeline = gfxinterface->create_graphics_pipeline(
        shader_stages,
        pipeline_state,
        descriptor_set_blueprints
    );

    constexpr std::array VERTICES = {
        Vertex{ { -0.5, -0.5, 0.0 }, { 1.0, 0.0, 0.0 } },
        Vertex{ {  0.0,  0.5, 0.0 }, { 0.0, 1.0, 0.0 } },
        Vertex{ {  0.5, -0.5, 0.0 }, { 0.0, 0.0, 1.0 } }
    };
    auto vertex_buffer = gfxinterface->create_buffer(
        sizeof(Vertex) * VERTICES.size(),
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    gfxinterface->write_buffer<Vertex>(vertex_buffer, VERTICES);

    constexpr std::array<uint16_t, 3> INDICES = { 0, 1, 2 };
    auto index_buffer = gfxinterface->create_buffer(
        sizeof(uint16_t) * INDICES.size(),
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    gfxinterface->write_buffer<uint16_t>(index_buffer, INDICES);

    window.set_visible(true);

    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }

        auto command_buffer = window.begin_rendering_frame();

        gfxinterface->record_command_buffer(command_buffer, [&](CommandRecorder& recorder) {
            recorder.bind_pipeline(pipeline);
            recorder.bind_vertex_buffer(vertex_buffer);
            recorder.bind_index_buffer(index_buffer, vk::IndexType::eUint16);
            recorder.draw_indexed(INDICES.size(), 1);
        });

        window.present_frame();
    }

    gfxinterface->wait_idle();
    gfxinterface->destroy_buffer(vertex_buffer);
    gfxinterface->destroy_buffer(index_buffer);
    gfxinterface->destroy_pipeline(pipeline);
    gfxinterface->destroy_descriptor_set_blueprints(descriptor_set_blueprints);
    for (auto shader : shaders) {
        gfxinterface->destroy_shader_module(shader);
    }
}

int main(int argc, char* args[]) {
    run();
    return 0;
}