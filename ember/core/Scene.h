#pragma once

#include "ember/ecs/World.h"

namespace ember::core {

    class Scene {
    public:
        virtual ~Scene() = default;

        ecs::World& world() { return m_world; }
        const ecs::World& world() const { return m_world; }

        virtual void on_enter() {}
        virtual void on_exit() {}

        virtual void update(float dt) {}
        virtual void render() {}
        virtual void render_ui() {}

    private:
        ecs::World m_world;
    };

}