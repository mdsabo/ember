#include "Projections.h"

#include <cmath>

namespace ember::graphics {

    Eigen::Matrix4f perspective_projection(
        float fovy,
        float aspect_ratio,
        float near,
        float far
    ) {
        const auto g = 1.0f / std::tanf(fovy * 0.5);
        const auto m00 = g / aspect_ratio;
        if (far == INFINITY) {
            // Infinite back plane
            // g/s 0  0  0
            //  0  g  0  0
            //  0  0  1 -n
            //  0  0  1  0
            return Eigen::Matrix4f({
                { m00, 0,   0,     0 },
                { 0,   g,   0,     0 },
                { 0,   0,   1, -near },
                { 0,   0,   1,     0 }
            });
        } else {
            const auto view_range = far - near;
            const auto m22 = far / view_range;
            const auto m23 = -m22 * near;
            return Eigen::Matrix4f({
                { m00, 0,   0,   0 },
                { 0,   g,   0,   0 },
                { 0,   0, m22, m23 },
                { 0,   0,   1,   0 }
            });
        }
    }

    Eigen::Matrix4f orthographic_projection(float fov) {
        return {};
    }

}