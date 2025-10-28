#pragma once

#include <concepts>

#include "World.h"

namespace ember::ecs {

    template<typename T>
    concept System = requires(World& world, float dt) {
        T::init(world);
        T::run(world, dt);
    };

    typedef void(*SystemCreateFn)(World& world);
    typedef void(*SystemRunFn)(World& world, float dt);

}