#pragma once

#include <Eigen/Dense>

namespace ember::geometry {

    struct Ray {
        Eigen::Vector3f origin;
        Eigen::Vector3f direction;
        float t;
    };

}