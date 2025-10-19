#pragma once

#include <Eigen/Dense>

namespace ember::geometry {

    struct Ray {
        Eigen::Vector3f origin;
        Eigen::Vector3f direction;
    };

    struct AABB {
        Eigen::Vector3f min;
        Eigen::Vector3f max;
    };

    struct Sphere {
        Eigen::Vector3f center;
        float radius;
    };

}