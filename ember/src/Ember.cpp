#include "Ember.h"

#include <SDL3/SDL.h>

#include "ember/gpu/GPUResource.h"
#include "ember/util/Log.h"

namespace ember {

    namespace {
        gpu::AppInfo make_gpu_app_info(const AppInfo& app_info) {
            constexpr gpu::AppInfo GPU_APP_INFO {
                .features {
#if NDEBUG
                    .vk_layer_validation = true,
                    // .vk_layer_api_dump = true
#endif
                }
            };

            auto gpu_app_info = GPU_APP_INFO;
            gpu_app_info.name = app_info.app_name;
            gpu_app_info.version = app_info.version;
            return gpu_app_info;
        }
    }

    Ember::Ember(
        int argc,
        char* argv[],
        const AppInfo& app_info,
        std::unique_ptr<core::Scene> first_scene
    ):
        m_argc(argc), m_argv(argv),
        m_vulkan_instance(gpu::VulkanInstance::create(make_gpu_app_info(app_info))),
        m_scene_manager(std::move(first_scene))

    {
        info(EMBER_LOG, "Hello, Ember");
        gpu::GPUResource::vulkan_instance = m_vulkan_instance;
    }

    int Ember::run() {
        while(m_scene_manager.current_scene()) {
            const auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::chrono::seconds::period> dt = current_time - m_last_world_update;

            m_scene_manager.current_scene()->world().run(dt.count());

            m_last_world_update = current_time;
        }

        return 0;
    }

}