#pragma once

#include <glm/glm.hpp>
#include <numbers>
#include <utility>

namespace ember::geometry {

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    struct Sphere {
        glm::vec3 center;
        float radius;

        inline float volume(const Sphere& s) {
            return 1.333333333333f * std::numbers::pi_v<float> * radius * radius * radius;
        }
    };

    struct AABB {
        glm::vec3 center;
        glm::vec3 half_size;

        struct Extent {
            glm::vec3 min;
            glm::vec3 max;
        };
        Extent extent() const {
            return { center - half_size, center + half_size };
        }
        static AABB from_extent(const Extent& extent) {
            const auto half_size = (extent.max - extent.min) * 0.5f;
            return AABB{
                .center = extent.min + half_size,
                .half_size = half_size
            };
        }

        float volume(const AABB& a) const {
            return half_size.x * half_size.y * half_size.z * 8;
        }
    };

}