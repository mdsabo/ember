#pragma once

#include "ember/ecs/Component.h"
#include "ember/ecs/Entity.h"
#include "ember/ecs/Storage.h"
#include "ember/gpu/RenderObjects.h"

namespace ember::graphics {

    struct MeshComponent {
        using Storage = ecs::DenseVectorStorage<MeshComponent>;

        ecs::Entity scene;      ///< Points to SceneComponent that has the mesh
        size_t mesh_id;         ///< Mesh index within the scene
    };

}