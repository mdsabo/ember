#pragma once

#include <Eigen/Dense>

namespace ember::graphics {

    struct Vertex {
        Eigen::Vector3f position;
        Eigen::Vector3f normal;
    };

}