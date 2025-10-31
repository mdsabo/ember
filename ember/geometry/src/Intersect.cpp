#include "Intersect.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace ember::geometry {

    bool intersect(const Sphere& s1, const Sphere& s2) {
        const auto min_intersect_dist = s1.radius + s2.radius;
        const auto min_dist2 = min_intersect_dist * min_intersect_dist;
        const auto dist2 = glm::distance2(s1.center, s2.center);
        return dist2 <= min_dist2;
    }

    namespace {
        bool contains(const AABB& a, glm::vec3 point) {
            return point.x >= a.min.x
                && point.y >= a.min.y
                && point.z >= a.min.z
                && point.x <= a.max.x
                && point.y <= a.max.y
                && point.z <= a.max.z;
        }
    }

    bool intersect(const Sphere& s, const AABB& a) {
        return contains(
            AABB{
                .min = a.min - s.radius,
                .max = a.max + s.radius
            },
            s.center
        );
    }

    bool intersect(const AABB& a1, const AABB& a2) {

    }

    std::pair<float, float> intersect(const Ray& ray, const Sphere& sphere) {
        return {};
    }

}