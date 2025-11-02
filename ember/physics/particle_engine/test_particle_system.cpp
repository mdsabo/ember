#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


#include "ParticleComponent.h"
#include "ParticleSystem.h"
#include "ember/ecs/World.h"

using namespace ember::ecs;
using namespace ember::physics;

#define REQUIRE_VEC_APPROX(vec, expected) \
    REQUIRE(vec.x == Catch::Approx(expected.x)); \
    REQUIRE(vec.y == Catch::Approx(expected.y)); \
    REQUIRE(vec.z == Catch::Approx(expected.z))

TEST_CASE("ParticleSystem::run skips entities with infinite mass (0 inverse mass)", "[ParticleSystem]") {
    auto world = World();
    ParticleSystem::init(world);

    auto e = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e, ParticleComponent {
            .inverse_mass = 0.0,
            .position = glm::vec3(),
            .velocity = glm::vec3(1.0, 2.0, 3.0),
        });
    }

    ParticleSystem::run(world, 2.0);

    {
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e).position, glm::vec3());
    }
}

TEST_CASE("ParticleSystem::run integrates particle position using dt and velocity", "[ParticleSystem]") {
    auto world = World();
    ParticleSystem::init(world);

    auto e = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e, ParticleComponent {
            .position = glm::vec3(1.0, 2.0, 3.0),
            .velocity = glm::vec3(1.0, 2.0, 3.0)
        });
    }

    ParticleSystem::run(world, 2.0);

    {
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e).position, glm::vec3(3.0, 6.0, 9.0));
    }
}

TEST_CASE("ParticleSystem::run integrates particle velocity using dt, acceleration and damping", "[ParticleSystem]") {
    auto world = World();
    ParticleSystem::init(world);

    auto e = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e, ParticleComponent {
            .velocity = glm::vec3(1.0, 2.0, 3.0),
            .acceleration = glm::vec3(1.0, 2.0, 3.0),
            .damping = 0.5
        });
    }

    ParticleSystem::run(world, 2.0);

    // See note in particle system velocity update but we're appling damping after acceleration is added
    // v' = (v + at) * (damping^t)
    // v' = (v + a*2.0) * (0.25)
    {
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e).velocity, glm::vec3(0.75, 1.5, 2.25));
    }
}

TEST_CASE("ParticleSystem::run applies forces to acceleration then clears forces", "[ParticleSystem]") {
    auto world = World();
    ParticleSystem::init(world);

    auto e = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e, ParticleComponent {
            .inverse_mass = 2.0,
            .velocity = glm::vec3(1.0, 2.0, 3.0),
            .acceleration = glm::vec3(1.0, 2.0, 3.0),
            .damping = 0.5,
            .applied_forces = glm::vec3(1.0, 2.0, 3.0)
        });
    }

    ParticleSystem::run(world, 2.0);

    // We've now added an impulse from the last test
    // a' = a + impulse * inverse_mass
    // v' = ... (using updated acceleration)
    {
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e).velocity, glm::vec3(1.75, 3.5, 5.25));
        REQUIRE_VEC_APPROX(particles.at(e).acceleration, glm::vec3(3.0, 6.0, 9.0));
        REQUIRE(particles.at(e).applied_forces == glm::vec3());
    }
}
