#pragma once

#include "ember/ecs/Component.h"
#include "ember/ecs/Storage.h"

namespace ember::graphics {

    struct MeshComponent {
        using Storage = ecs::DenseVectorStorage<MeshComponent>;
        uint64_t mesh_id;
    };

}