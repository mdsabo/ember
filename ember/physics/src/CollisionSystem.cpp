#include "CollisionSystem.h"

#include <array>
#include <glm/glm.hpp>

#include "ColliderComponent.h"
#include "RigidBodyComponent.h"

#include "ember/ecs/TransformComponent.h"

namespace ember::physics {

    void CollisionSystem::init(ecs::World& world) {
        world.add_component<ColliderComponent>();
    }

    void CollisionSystem::run(ecs::World& world, float) {

    }


}