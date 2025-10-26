#pragma once

#include <glm/glm.hpp>

namespace ember::geometry {

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };

    struct Sphere {
        glm::vec3 center;
        float radius;
    };

}