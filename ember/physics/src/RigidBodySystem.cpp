#include "RigidBodySystem.h"

#include <cassert>

#include "RigidBodyComponent.h"
#include "ember/ecs/TransformComponent.h"

namespace ember::physics {

    void RigidBodySystem::init(ecs::World& world) {
        world.add_component<RigidBodyComponent>();
    }

    namespace {
        void update_rigid_body(
            RigidBodyComponent& body,
            const ecs::TransformComponent& transform,
            float dt
        ) {
            body.acceleration += body.applied_force * body.inverse_mass;

            // FIXME: What about scaling on the transform????
            const auto world_iit = body.get_world_space_iit(transform.transform);
            const auto angular_accel = body.applied_torque * world_iit;

            body.velocity += body.acceleration * dt;
            body.velocity *= std::powf(body.linear_damping, dt);
            body.angular_velocity += angular_accel * dt;
            body.angular_velocity *= std::powf(body.angular_damping, dt);

            body.position += body.velocity * dt;
            body.rotation = glm::rotate(body.rotation, dt, body.angular_velocity);

            body.applied_force = body.applied_torque = glm::vec3();
        }
    }

    void RigidBodySystem::run(ecs::World& world, float dt) {
        assert(dt > 0.0f);

        auto& rigid_bodies = world.write_component<RigidBodyComponent>();
        const auto& transforms = world.read_component<ecs::TransformComponent>();

        for (auto& rigid_body : rigid_bodies) {
            update_rigid_body(rigid_body, transforms.at(0), dt); // FIXME component iteration
        }
    }


}