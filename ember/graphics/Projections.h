#pragma once

#include <Eigen/Dense>

namespace ember::graphics {

    Eigen::Matrix4f perspective_projection(
        float fovy,
        float aspect_ratio,
        float near,
        float far = INFINITY
    );
    Eigen::Matrix4f orthographic_projection(float fov);

}