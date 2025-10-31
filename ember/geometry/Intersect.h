#pragma once

#include <utility>

#include "Shapes.h"

namespace ember::geometry {

    bool intersect(const Sphere& s1, const Sphere& s2);
    bool intersect(const Sphere& s, const AABB& a);
    bool intersect(const AABB& a1, const AABB& b);

    // std::pair<float, float> intersect(const Ray& ray, const Sphere& sphere);

}