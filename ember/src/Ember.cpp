#include "Ember.h"

#include <SDL3/SDL.h>

#include "ember/util/Log.h"

namespace ember {

    void Ember::create_graphics_objects(const AppInfo& app_info) {
        m_vulkan = gpu::VulkanInstance::create({
            .features {
#if NDEBUG
                .vk_layer_validation = true,
                // .vk_layer_api_dump = true
#endif
            }
        });
    }

    Ember::Ember(
        int argc,
        char* argv[],
        const AppInfo& app_info,
        std::unique_ptr<core::Scene> first_scene
    ):
        m_argc(argc), m_argv(argv), m_scene_manager(std::move(first_scene))
    {
        info(EMBER_LOG, "Hello, Ember");
        create_graphics_objects(app_info);
    }

    int Ember::run() {
        m_window->set_visible(true);

        while(m_scene_manager.current_scene()) {
            const auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::chrono::seconds::period> dt = current_time - m_last_world_update;
            m_scene_manager.current_scene()->world().run(dt.count());
        }

        return 0;
    }

}