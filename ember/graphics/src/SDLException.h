#pragma once

#include <SDL3/SDL.h>

#include <format>
#include <stdexcept>
#include <string_view>

namespace ember::graphics {

    class SDLException : public std::runtime_error {
    public:
        SDLException(const std::string& msg): std::runtime_error(
            std::format("SDLException : {} : ", msg, SDL_GetError())
        ) {}
    };

}