#include <catch2/catch_test_macros.hpp>

#include "glm_compare.h"
#include "RigidBodyComponent.h"

using namespace ember::physics;

TEST_CASE("RigidBodyComponent::get_transform returns homogenous transform of trasnlation and rotation", "[RigidBodyComponent]") {
    const RigidBodyComponent rbc {
        .position = glm::vec3(1.0, 2.0, 3.0),
        .rotation = glm::angleAxis(60.0f, glm::vec3(0.0, 1.0, 0.0)),
    };

    auto expected = glm::identity<glm::mat4>();
    expected = glm::translate(expected, rbc.position);
    expected = glm::rotate(expected, 60.0f, glm::vec3(0.0, 1.0, 0.0));

    const auto transform = rbc.get_transform();
    REQUIRE_MAT4_REL(transform, expected, 0.001f);
}

TEST_CASE("RigidBodyComponent::get_world_space_iit", "[RigidBodyComponent]") {
    RigidBodyComponent rbc;
    rbc.set_inertia_tensor(glm::mat3(
        glm::vec3(3.0, 0.0, 0.0),
        glm::vec3(0.0, 4.0, 0.0),
        glm::vec3(0.0, 0.0, 1.0)
    ));

    const auto rotation = glm::rotate(
        glm::identity<glm::mat4>(),
        42.0f,
        glm::normalize(glm::vec3(1.0, 1.0, 1.0))
    );

    const auto rotation3 = glm::mat3(rotation);
    const auto expected = rotation3 * rbc.inverse_inertia_tensor * glm::inverse(rotation3);

    const auto iit = rbc.get_world_space_iit(rotation);
    REQUIRE_MAT3_REL(iit, expected, 0.001f);
}
