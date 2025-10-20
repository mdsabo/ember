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

    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }
    }
}

int main(int argc, char* args[]) {
    run();
    return 0;
}