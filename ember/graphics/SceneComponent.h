#pragma once

#include <assimp/scene.h>
#include <string>
#include <vector>

#include "ember/ecs/Component.h"
#include "ember/ecs/Storage.h"
#include "ember/gpu/Renderer.h"

namespace ember::graphics {

    struct SceneComponent {
        using Storage = ecs::DenseVectorStorage<SceneComponent>;

        struct Mesh {
            gpu::Buffer* vertex_buffer;
            gpu::Buffer* index_buffer;
            uint32_t index_count;
        };
        std::vector<Mesh> meshes;

        SceneComponent() = default;
        SceneComponent(const aiScene* scene, gpu::Renderer* renderer);
    };

}