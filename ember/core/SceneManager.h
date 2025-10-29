#pragma once

#include <memory>
#include <stack>

#include "Scene.h"

namespace ember::core {

    class SceneManager {
    public:
        SceneManager(std::unique_ptr<Scene> first_scene);

        Scene* current_scene() const;
        inline bool empty() const { return m_scenes.empty(); }

        void push_scene(std::unique_ptr<Scene> scene);
        void pop_scene();

    private:
        std::stack<std::unique_ptr<Scene>> m_scenes;

        void exit_current_scene();
        void enter_current_scene();
    };

}