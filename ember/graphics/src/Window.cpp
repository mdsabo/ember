#include "Window.h"

#include "src/SDLException.h"

namespace ember::graphics {

    Window::Window(const char* title, int w, int h, SDL_WindowFlags flags) {
        m_window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_VULKAN);
        if (m_window == nullptr) {
            throw SDLException(std::format(
                "Failed to create window {} with size ({}, {})", title, w, h
            ));
        };
    }

    Window::~Window() {
        SDL_DestroyWindow(m_window);
    }

    Window::Id Window::id() {
        return SDL_GetWindowID(m_window);
    }

}