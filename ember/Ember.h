#pragma once

#include <chrono>

#include "ember/core/SceneManager.h"
#include "ember/gpu/GPUDevice.h"
#include "ember/gpu/VulkanInstance.h"
#include "ember/graphics/Window.h"

namespace ember {

    struct AppInfo {
        const char* app_name;
        uint32_t version;

        std::unique_ptr<core::Scene> first_scene;
        graphics::WindowSettings window;
    };

    class Ember {
    public:
        Ember(
            int argc,
            char* argv[],
            const AppInfo& app_info,
            std::unique_ptr<core::Scene> first_scene
        );
        int run();

    private:
        int m_argc;
        const char* const* m_argv;
        std::chrono::high_resolution_clock::time_point m_last_fixed_update;

        std::shared_ptr<const gpu::VulkanInstance> m_vulkan;
        std::shared_ptr<const gpu::GPUDevice> m_gpu;
        std::unique_ptr<graphics::Window> m_window;

        core::SceneManager m_scene_manager;

        void create_graphics_objects(const AppInfo& app_info);

        bool process_events();
        void fixed_update();
        void render();
    };

}