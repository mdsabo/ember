#pragma once

#include <variant>

#include "ember/ecs/Component.h"
#include "ember/geometry/Shapes.h"

namespace ember::physics {

    struct ColliderComponent {
        using Storage = ecs::DenseVectorStorage<ColliderComponent>;
        std::variant<geometry::Sphere, geometry::AABB> shape;

        geometry::AABB aabb() const;
    };
    static_assert(ecs::Component<ColliderComponent>);

}