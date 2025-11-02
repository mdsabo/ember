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

    std::optional<IntersectInfo> intersect_info(const Sphere& s1, const Sphere& s2) {
        const auto min_intersect_dist = s1.radius + s2.radius;
        const auto min_dist2 = min_intersect_dist * min_intersect_dist;
        const auto dist2 = glm::distance2(s1.center, s2.center);
        if (dist2 <= min_dist2) {
            const auto dist = std::sqrt(dist2);
            return IntersectInfo {
                .normal = (s2.center - s1.center) / dist,
                .depth = min_intersect_dist - dist
            };
        } else {
            return std::nullopt;
        }
    }

    bool intersect(const AABB& a1, const AABB& a2) {
        // SAT test over each axis
        // If the abs distance from center to center is less than the sum of half_widths
        // then there must be some overlap on that axis, continue to the next axis
        // If no axis can separate them, then they must intersect
        const auto dx = a1.center.x - a2.center.x;
        if (std::abs(dx) >= (a1.half_size.x + a2.half_size.x)) return false;

        const auto dy = a1.center.y - a2.center.y;
        if (std::abs(dy) >= (a1.half_size.y + a2.half_size.y)) return false;

        const auto dz = a1.center.z - a2.center.z;
        if (std::abs(dz) >= (a1.half_size.z + a2.half_size.z)) return false;

        // No separating axis, they intersect
        return true;
    }

    // namespace {
    //     bool contains(const AABB& a, glm::vec3 point) {
    //         return point.x >= a.min.x
    //             && point.y >= a.min.y
    //             && point.z >= a.min.z
    //             && point.x <= a.max.x
    //             && point.y <= a.max.y
    //             && point.z <= a.max.z;
    //     }
    // }

    // std::optional<ContactInfo> intersect(const Sphere& s, const AABB& a) {
    //     const auto bounds = AABB{
    //         .min = a.min - s.radius,
    //         .max = a.max + s.radius
    //     };
    //     if (contains(bounds, s.center)) {
    //         auto norm = glm::vec3(1.0, 0.0, 0.0);
    //         if (a.min.y <= s.center.y && s.center.y <= a.max.y)
    //         return ContactInfo {
    //             .normal = glm::vec3(),
    //             .depth =
    //         }
    //     } else {
    //         return std::nullopt;
    //     }
    // }

    // std::optional<ContactInfo> intersect(const AABB& a1, const AABB& a2) {

    // }

    // std::pair<float, float> intersect(const Ray& ray, const Sphere& sphere) {
    //     return {};
    // }

}