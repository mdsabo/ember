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

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR).append("examples");
    auto vertex_shader_path = shader_path.append("sdl_window.vert");
    auto fragment_shader_path = shader_path.append("sdl_window.frag");

    std::array shaders = {
        renderer->create_shader_module(vertex_shader_path),
        renderer->create_shader_module(vertex_shader_path)
    };

    auto descriptor_set_blueprints = renderer->create_descriptor_set_blueprints(shaders);

    std::array<Renderer::ShaderStageInfo, 2> shader_stages = {
        Renderer::ShaderStageInfo{ .module = shaders[0] },
        Renderer::ShaderStageInfo{ .module = shaders[1] }
    };
    // auto pipeline = renderer->create_graphics_pipeline(
    //     shader_stages,
    //     Renderer::GraphicsPipelineState{
    //     }
    // );


    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }
    }

    for (auto& shader : shaders) {
        renderer->destroy_shader_module(shader);
    }
}

int main(int argc, char* args[]) {
    run();
    return 0;
}