#pragma once

#include "ember/ecs/Component.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Storage.h"
#include "ember/ecs/System.h"

namespace ember::physics {

    struct SpringComponent {
        using Storage = ecs::DenseVectorStorage<SpringComponent>;

        ecs::Entity e1, e2;
        float k;
        float rest_length;
        float maximum_distance = std::numeric_limits<float>::max();
    };
    static_assert(ecs::Component<SpringComponent>);

    class SpringSystem {
    public:
        static void init(ecs::World& world);
        static void run(ecs::World& world, float dt);
    };
    static_assert(ecs::System<SpringSystem>);

}