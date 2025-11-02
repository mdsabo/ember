#pragma once

#include "ember/ecs/System.h"
#include "ember/ecs/World.h"

namespace ember::physics {

    class RigidBodySystem {
    public:
        static void init(ecs::World& world);
        static void run(ecs::World& world, float dt);
    };

}