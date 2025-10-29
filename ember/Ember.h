#pragma once

#include <chrono>

#include "ember/core/SceneManager.h"
#include "ember/gpu/VulkanInstance.h"

namespace ember {

    struct AppInfo {
        const char* app_name;
        uint32_t version;
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
        std::chrono::high_resolution_clock::time_point m_last_world_update;
        std::shared_ptr<const gpu::VulkanInstance> m_vulkan_instance;

        core::SceneManager m_scene_manager;

    };

}