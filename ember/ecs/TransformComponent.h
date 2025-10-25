#pragma once

#include <Eigen/Dense>
#include "Storage.h"

namespace ember::ecs {

    struct TransformComponent {
        using Storage = VectorStorage<TransformComponent>;
        Eigen::Affine3f transform;
    };

}