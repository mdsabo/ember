#include "ColliderComponent.h"

namespace ember::physics {

    struct AABBVisitor {
        geometry::AABB operator()(const geometry::Sphere& sphere) {
            const auto half_size = glm::vec3(sphere.radius, sphere.radius, sphere.radius);
            return geometry::AABB {
                .center = sphere.center,
                .half_size = half_size
            };
        }
        geometry::AABB operator()(const geometry::AABB& aabb) {
            return aabb;
        }
    };

    geometry::AABB ColliderComponent::aabb() const {
        return std::visit(AABBVisitor(), shape);
    }

}