#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Component.h"
#include "Storage.h"

namespace ember::ecs {

    struct TransformComponent {
        using Storage = VectorStorage<TransformComponent>;
        glm::mat4 transform = glm::identity<glm::mat4>();
    };
    static_assert(Component<TransformComponent>);

}