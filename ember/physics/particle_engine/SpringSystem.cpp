#include "SpringSystem.h"

#include <cassert>

#include "ParticleComponent.h"
#include "ember/ecs/TransformComponent.h"
#include "ember/ecs/World.h"

namespace ember::physics {
    void SpringSystem::init(ecs::World& world) {
        world.add_component<ParticleComponent>();
        world.add_component<SpringComponent>();
    }

    void SpringSystem::run(ecs::World& world, float) {
        const auto& springs = world.read_component<SpringComponent>();
        auto& particles = world.write_component<ParticleComponent>();

        for (const auto& spring : springs) {
            assert(spring.maximum_distance >= spring.rest_length);
            assert(particles.contains(spring.e1) && particles.contains(spring.e2));

            const auto p1 = particles.at(spring.e1).position;
            const auto p2 = particles.at(spring.e2).position;

            // The difference vector points from p1 to p2 which means the force is oriented for p1.
            // To get the force on p2 we will invert the force.
            const auto diff = p2 - p1;
            const auto distance = glm::length(diff);
            const auto spring_distance = std::min(distance, spring.maximum_distance) - spring.rest_length;
            if (spring_distance <= 0.0f) continue;

            const auto force = spring.k * spring_distance * (diff / distance);

            particles.at(spring.e1).applied_forces += force;
            particles.at(spring.e2).applied_forces -= force;
        }
    }
}
