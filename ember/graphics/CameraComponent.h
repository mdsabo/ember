#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "ember/ecs/Component.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Storage.h"

namespace ember::graphics {

    struct CameraComponent {
        using Storage = ecs::MapStorage<CameraComponent>;

        static constexpr auto UP_VECTOR = glm::vec3(0.0, 1.0, 0.0);

        ecs::Entity focal_point;
        glm::mat4 projection;
        vk::Viewport viewport;
    };

}