#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ember/ecs/Component.h"

namespace ember::physics {

    struct RigidBodyComponent {
        using Storage = ecs::DenseVectorStorage<RigidBodyComponent>;

        float inverse_mass;

        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 acceleration;
        glm::vec3 applied_force;
        float linear_damping;

        glm::quat rotation;
        glm::vec3 angular_velocity;
        glm::mat3 inverse_inertia_tensor;
        glm::vec3 applied_torque;
        float angular_damping;

        bool is_awake;

        void set_rotation(const glm::quat& rotation);

        glm::mat4x3 get_transform() const;

        void set_inertia_tensor(const glm::mat3& intertia);
        glm::mat3 get_world_space_iit(const glm::mat4& world_transform) const;

        /// @brief Add a force to the body at the center of mass (torque applied = 0)
        /// @param force Force vector
        void add_force(const glm::vec3& force);
        /// @brief Add a force to the body at a point in world space (may result in torque)
        /// @param force Force vector
        /// @param location Location of applied force
        void add_force(const glm::vec3& force, const glm::vec3& location);
    };
    static_assert(ecs::Component<RigidBodyComponent>);

}