#include <array>
#include <catch2/catch_test_macros.hpp>

#include "SceneManager.h"

using namespace ember::core;

struct TestScene final : public Scene {
    int id = 0;
    TestScene(int id): id(id) {}

    bool on_enter_called = false;
    virtual void on_enter() {
        on_enter_called = true;
    }

    bool on_exit_called = false;
    virtual void on_exit() {
        on_exit_called = true;
    }
};

TEST_CASE("SceneManager::SceneManager sets arg as current scene", "[SceneManager]") {
    auto scene = std::make_unique<TestScene>(42);
    auto scene_manager = SceneManager(std::move(scene));

    auto current_scene = reinterpret_cast<TestScene*>(scene_manager.current_scene());

    REQUIRE(current_scene != nullptr);
    REQUIRE(current_scene->id == 42);
    REQUIRE(current_scene->on_enter_called);
}

TEST_CASE("SceneManager::push_scene changes to provided scene and calls enter/exit callbacks", "[SceneManager]") {
    auto scene1 = std::make_unique<TestScene>(1);
    auto scene1_ptr = scene1.get();

    auto scene2 = std::make_unique<TestScene>(2);
    auto scene2_ptr = scene2.get();

    auto scene_manager = SceneManager(std::move(scene1));
    scene_manager.push_scene(std::move(scene2));

    REQUIRE(scene1_ptr->on_exit_called);
    REQUIRE(scene2_ptr->on_enter_called);
    REQUIRE(scene_manager.current_scene() == scene2_ptr);
}

TEST_CASE("SceneManager::pop_scene changes to previous scene and calls enter/exit callbacks", "[SceneManager]") {
    auto scene1 = std::make_unique<TestScene>(1);
    auto scene1_ptr = scene1.get();

    auto scene2 = std::make_unique<TestScene>(2);
    auto scene2_ptr = scene2.get();

    auto scene_manager = SceneManager(std::move(scene1));
    scene_manager.push_scene(std::move(scene2));

    scene1_ptr->on_enter_called = false;

    scene_manager.pop_scene();

    REQUIRE(scene1_ptr->on_enter_called);
    // REQUIRE(scene2_ptr->on_enter_called); Can't look at scene2 since it was destroyed
    REQUIRE(scene_manager.current_scene() == scene1_ptr);
}

TEST_CASE("SceneManager::current_scene returns nullptr if no scenes exist", "[SceneManager]") {
    auto scene1 = std::make_unique<TestScene>(1);
    auto scene_manager = SceneManager(std::move(scene1));
    scene_manager.pop_scene();

    REQUIRE(scene_manager.current_scene() == nullptr);
}
