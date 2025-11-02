#pragma once

#include "ember/ecs/Component.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Storage.h"

namespace ember::physics {

    struct ParticleDragComponent {
        using Storage = ecs::DenseVectorStorage<ParticleDragComponent>;

        float k1, k2;
    };
    static_assert(ecs::Component<ParticleDragComponent>);

}