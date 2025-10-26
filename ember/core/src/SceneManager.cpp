#include "SceneManager.h"

#include <cassert>

#include "ember/util/Log.h"

namespace ember::core {

    SceneManager::SceneManager(std::unique_ptr<Scene> first_scene) {
        push_scene(std::move(first_scene));
    }

    Scene* SceneManager::current_scene() const {
        if (empty()) return nullptr;
        else return m_scenes.top().get();
    }

    void SceneManager::exit_current_scene() {
        auto scene = current_scene();
        if (scene) {
            info(EMBER_CORE_LOG, "Exiting scene {:p}", (void*)scene);
            scene->on_exit();
        }
    }
    void SceneManager::enter_current_scene() {
        auto scene = current_scene();
        if (scene) {
            info(EMBER_CORE_LOG, "Entering scene {:p}", (void*)scene);
            scene->on_enter();
        }
    }

    void SceneManager::push_scene(std::unique_ptr<Scene> scene) {
        assert(scene != nullptr);
        exit_current_scene();
        m_scenes.push(std::move(scene));
        enter_current_scene();
    }

    void SceneManager::pop_scene() {
        assert(!empty());
        exit_current_scene();
        m_scenes.pop();
        enter_current_scene();
    }

    void SceneManager::update(float dt) {
        if (current_scene()) current_scene()->update(dt);
    }

    void SceneManager::render() {
        if (current_scene()) current_scene()->render();
    }


}