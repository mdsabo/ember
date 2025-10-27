#pragma once

#include <functional>
#incldue <vector>

#include "ember/ecs/World.h"
#include "ember/graphics/Renderer.h"

namespace ember::core {

    enum class SystemPhases {
        PreUpdate,
        Update,
        PostUpdate,
        PreRender,
        Render,
        RenderUI
    };

    using PreUpdateSystem = std::function<void(ecs::World&)>;
    using UpdateSystem = std::function<void(ecs::World&, float)>;
    using PostUpdateSystem = std::function<void(ecs::World&)>;
    using PreRenderSystem = std::function<void(ecs::World&)>;
    using RenderSystem = std::function<void(ecs::World&, Renderer&)>;
    using RenderUISystem = std::function<void(ecs::World&, Renderer&)>;

    struct Systems {
        std::vector<PreUpdateSystem> preupdate;
        std::vector<UpdateSystem> update;
        std::vector<PostUpdateSystem> postupdate;
        std::vector<PreRenderSystem> prerender;
        std::vector<RenderSystem> render;
        std::vector<RenderUISystem> renderui;
    };

}