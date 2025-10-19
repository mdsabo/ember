#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/SDLException.h"

#include "Window.h"

using namespace ember::graphics;

void run() {
    if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed SDL_INIT_VIDEO");

    auto window = Window("SDL Playground", 800, 600);

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