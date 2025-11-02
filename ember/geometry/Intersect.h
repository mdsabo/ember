#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <utility>

#include "Shapes.h"

namespace ember::geometry {

    struct IntersectInfo {
        glm::vec3 normal;
        float depth;
    };

    bool intersect(const Sphere& s1, const Sphere& s2);
    std::optional<IntersectInfo> intersect_info(const Sphere& s1, const Sphere& s2);

    bool intersect(const AABB& a1, const AABB& a2);

    //  intersect(const Sphere& s1, const Sphere& s2);
    // std::optional<ContactInfo> intersect(const Sphere& s, const AABB& a);
    // std::optional<ContactInfo> intersect(const AABB& a1, const AABB& b);

    // std::pair<float, float> intersect(const Ray& ray, const Sphere& sphere);

}