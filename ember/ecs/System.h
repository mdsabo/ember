#pragma once

#include <concepts>

#include "World.h"

namespace ember::ecs {

    template<typename T>
    concept System = requires(World& world) {
        T::init(world);
    };

}