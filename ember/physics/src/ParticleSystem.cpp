#include "ParticleSystem.h"

#include <cassert>

#include "ParticleComponent.h"
#include "ember/ecs/World.h"

namespace ember::physics {
    void ParticleSystem::init(ecs::World& world) {
        world.add_component<ParticleComponent>();
    }

    namespace {
        void update_particle(ParticleComponent& particle, float dt) {
            if (particle.inverse_mass <= 0.0) return;

            particle.position += particle.velocity * dt;

            particle.acceleration += particle.applied_impulses * particle.inverse_mass;
            particle.applied_impulses = glm::vec3();

            // There's a bit of an error in the book here it would appear.
            // It states velocity should be v' = v*(damping^dt) + a*t
            // but the code listng applies damping after adding acceleration
            // I'm following the code listing because it probably doesn't matter
            // much, but interesting nonetheless.
            particle.velocity += (particle.acceleration * dt);
            particle.velocity *= std::powf(particle.damping, dt);
        }
    }

    void ParticleSystem::run(ecs::World& world, float dt) {
        assert(dt > 0.0f);

        auto& particles = world.write_component<ParticleComponent>();

        for (auto& particle : particles) {
            update_particle(particle, dt);
        }
    }
}
