#pragma once

#include <Eigen/Dense>
#include <vulkan/vulkan.hpp>

#include "ember/ecs/Component.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Storage.h"

namespace ember::graphics {

    struct CameraComponent {
        using Storage = ecs::MapStorage<CameraComponent>;

        ecs::Entity focal_point;
        Eigen::Matrix4f projection;
        vk::Viewport viewport;
    };

}