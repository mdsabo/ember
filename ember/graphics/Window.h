#pragma once

#include <SDL3/SDL.h>

namespace ember::graphics {

    class Window {
    public:
        using Id = SDL_WindowID;

        Window(const char* title, int w, int h, SDL_WindowFlags flags = 0);
        ~Window();

        Id id();
    private:
        SDL_Window* m_window;
    };

}