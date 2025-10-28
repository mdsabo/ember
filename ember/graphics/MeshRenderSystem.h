#pragma once

#include "Renderer.h"
#include "ember/ecs/System.h"
#include "ember/ecs/World.h"

namespace ember::graphics {

    class MeshRenderSystem {
    public:
        static void init(ecs::World& world);
        static void run(ecs::World& world, float dt);
        void render(ecs::World& world, Renderer& renderer);
    };
    static_assert(ecs::System<MeshRenderSystem>);

}
