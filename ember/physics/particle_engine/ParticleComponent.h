#pragma once

#include <glm/glm.hpp>

#include "ember/ecs/Component.h"
#include "ember/ecs/Storage.h"

namespace ember::physics {

    struct ParticleComponent {
        using Storage = ecs::DenseVectorStorage<ParticleComponent>;

        float inverse_mass = 1.0f;

        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 acceleration;
        float damping = 1.0f;

        glm::vec3 applied_forces;
    };
    static_assert(ecs::Component<ParticleComponent>);
}