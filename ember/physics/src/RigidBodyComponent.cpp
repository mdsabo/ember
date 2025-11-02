#include "RigidBodyComponent.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ember::physics {

    void RigidBodyComponent::set_rotation(const glm::quat& rotation) {
        this->rotation = glm::normalize(rotation);
    }

    glm::mat4x3 RigidBodyComponent::get_transform() const {
        glm::mat4x3 transform = glm::mat3_cast(rotation);

        transform[3][0] = position.x;
        transform[3][1] = position.y;
        transform[3][2] = position.z;

        return transform;
    }

    void RigidBodyComponent::set_inertia_tensor(const glm::mat3& inertia) {
        inverse_inertia_tensor = glm::inverse(inertia);
    }

    glm::mat3 RigidBodyComponent::get_world_space_iit(const glm::mat4& world_rotation) const {
        // See Game Engine Physics Development pg. 218
        // This is a highly-optimized version of what's in the unit test that makes some
        // assumptions about trasnforms (e.g. inverse of rotation matrix is the transpose)
        float t4 = world_rotation[0][0] * inverse_inertia_tensor[0][0]
            + world_rotation[1][0] * inverse_inertia_tensor[0][1]
            + world_rotation[2][0] * inverse_inertia_tensor[0][2];
        float t9 = world_rotation[0][0] * inverse_inertia_tensor[1][0]
            + world_rotation[1][0] * inverse_inertia_tensor[1][1]
            + world_rotation[2][0] * inverse_inertia_tensor[1][2];
        float t14 = world_rotation[0][0] * inverse_inertia_tensor[2][0]
            + world_rotation[1][0] * inverse_inertia_tensor[2][1]
            + world_rotation[2][0] * inverse_inertia_tensor[2][2];

        float t28 = world_rotation[0][1] * inverse_inertia_tensor[0][0]
            + world_rotation[1][1] * inverse_inertia_tensor[0][1]
            + world_rotation[2][1] * inverse_inertia_tensor[0][2];
        float t33 = world_rotation[0][1] * inverse_inertia_tensor[1][0]
            + world_rotation[1][1] * inverse_inertia_tensor[1][1]
            + world_rotation[2][1] * inverse_inertia_tensor[1][2];
        float t38 = world_rotation[0][1] * inverse_inertia_tensor[2][0]
            + world_rotation[1][1] * inverse_inertia_tensor[2][1]
            + world_rotation[2][1] * inverse_inertia_tensor[2][2];

        float t52 = world_rotation[0][2] * inverse_inertia_tensor[0][0]
            + world_rotation[1][2] * inverse_inertia_tensor[0][1]
            + world_rotation[2][2] * inverse_inertia_tensor[0][2];
        float t57 = world_rotation[0][2] * inverse_inertia_tensor[1][0]
            + world_rotation[1][2] * inverse_inertia_tensor[1][1]
            + world_rotation[2][2] * inverse_inertia_tensor[1][2];
        float t62 = world_rotation[0][2] * inverse_inertia_tensor[2][0]
            + world_rotation[1][2] * inverse_inertia_tensor[2][1]
            + world_rotation[2][2] * inverse_inertia_tensor[2][2];

        glm::mat3 iit;
        iit[0][0] = t4 * world_rotation[0][0]
            + t9 * world_rotation[1][0]
            + t14 * world_rotation[2][0];
        iit[1][0] = t4 * world_rotation[0][1]
            + t9 * world_rotation[1][1]
            + t14 * world_rotation[2][1];
        iit[2][0] = t4 * world_rotation[0][2]
            + t9 * world_rotation[1][2]
            + t14 * world_rotation[2][2];

        iit[0][1] = t28 * world_rotation[0][0]
            + t33 * world_rotation[1][0]
            + t38 * world_rotation[2][0];
        iit[1][1] = t28 * world_rotation[0][1]
            + t33 * world_rotation[1][1]
            + t38 * world_rotation[2][1];
        iit[2][1] = t28 * world_rotation[0][2]
            + t33 * world_rotation[1][2]
            + t38 * world_rotation[2][2];

        iit[0][2] = t52 * world_rotation[0][0]
            + t57 * world_rotation[1][0]
            + t62 * world_rotation[2][0];
        iit[1][2] = t52 * world_rotation[0][1]
            + t57 * world_rotation[1][1]
            + t62 * world_rotation[2][1];
        iit[2][2] = t52 * world_rotation[0][2]
            + t57 * world_rotation[1][2]
            + t62 * world_rotation[2][2];

        return iit;
    }

    void RigidBodyComponent::add_force(const glm::vec3& force) {
        applied_force += force;
        is_awake = true;
    }

    void RigidBodyComponent::add_force(const glm::vec3& force, const glm::vec3& location) {
        add_force(force);
        applied_torque += glm::cross(location - position, force);
    }

}