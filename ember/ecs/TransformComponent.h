#pragma once

#include <glm/glm.hpp>
#include "Storage.h"

namespace ember::ecs {

    struct TransformComponent {
        using Storage = VectorStorage<TransformComponent>;
        glm::mat4 transform = glm::identity<glm::mat4>();
    };

}