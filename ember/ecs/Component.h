#pragma once

#include <concepts>

namespace ember::ecs {

    template<typename T>
    concept Component = requires() {
        typename T::Storage;
    };

}