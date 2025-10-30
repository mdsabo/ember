#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "ParticleComponent.h"
#include "SpringSystem.h"
#include "ember/ecs/World.h"

using namespace ember::ecs;
using namespace ember::physics;

#define REQUIRE_VEC_APPROX(vec, expected) \
    REQUIRE(vec.x == Catch::Approx(expected.x)); \
    REQUIRE(vec.y == Catch::Approx(expected.y)); \
    REQUIRE(vec.z == Catch::Approx(expected.z))

TEST_CASE("SpringSystem::run applies spring force to particle components", "[SpringSystem]") {
    auto world = World();
    SpringSystem::init(world);

    auto e1 = world.create_entity();
    auto e2 = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e1, ParticleComponent { .position = glm::vec3() });
        particles.insert(e2, ParticleComponent { .position = glm::vec3(10.0, 10.0, 10.0) });

        auto& springs = world.write_component<SpringComponent>();
        springs.insert(e1, SpringComponent{
            .e1 = e1,
            .e2 = e2,
            .k = 0.5,
            .rest_length = 1.0
        });
    }

    SpringSystem::run(world, 2.0);

    {
        // force = k * (distance - rest_length) * norm(p1 - p2) = 0.5 * (sqrt(300) - 1.0) * ((10, 10, 10) / sqrt(300))
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e1).applied_impulses, glm::vec3(4.71132, 4.71132, 4.71132));
        REQUIRE_VEC_APPROX(particles.at(e2).applied_impulses, glm::vec3(-4.71132, -4.71132, -4.71132));
    }
}

TEST_CASE("SpringSystem::run does not increase force after max_distance", "[SpringSystem]") {
    auto world = World();
    SpringSystem::init(world);

    auto e1 = world.create_entity();
    auto e2 = world.create_entity();
    {
        auto& particles = world.write_component<ParticleComponent>();
        particles.insert(e1, ParticleComponent { .position = glm::vec3() });
        particles.insert(e2, ParticleComponent { .position = glm::vec3(10.0, 10.0, 10.0) });

        auto& springs = world.write_component<SpringComponent>();
        springs.insert(e1, SpringComponent{
            .e1 = e1,
            .e2 = e2,
            .k = 0.5,
            .rest_length = 1.0,
            .maximum_distance = 5.0
        });
    }

    SpringSystem::run(world, 2.0);

    {
        // force = k * (distance - rest_length) * norm(p1 - p2) = 0.5 * (5.0 - 1.0) * ((10, 10, 10) / sqrt(300))
        const auto& particles = world.read_component<ParticleComponent>();
        REQUIRE_VEC_APPROX(particles.at(e1).applied_impulses, glm::vec3(1.1547, 1.1547, 1.1547));
        REQUIRE_VEC_APPROX(particles.at(e2).applied_impulses, glm::vec3(-1.1547, -1.1547, -1.1547));
    }
}
