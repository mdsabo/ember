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

void run() {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());

    auto vkinstance = VulkanInstance::create({});
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
        .colorWriteMask = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA,
        .blendEnable = vk::False,
    };

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
            .lineWidth = 1.0,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
        },
        .multisample_state = vk::PipelineMultisampleStateCreateInfo {
            .sampleShadingEnable = vk::False,
            .rasterizationSamples = vk::SampleCountFlagBits::e1
        },
        .color_blend_state = vk::PipelineColorBlendStateCreateInfo {
            .logicOpEnable = vk::False,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment
        }
    };
    auto pipeline = renderer->create_graphics_pipeline(
        shader_stages,
        pipeline_state,
        descriptor_set_blueprints
    );


    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }
    }

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