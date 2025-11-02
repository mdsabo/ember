#pragma once

#include "ember/ecs/System.h"

namespace ember::physics {

    class ParticleSystem {
    public:
        static void init(ecs::World& world);
        static void run(ecs::World& world, float dt);
    };
    static_assert(ecs::System<ParticleSystem>);

}