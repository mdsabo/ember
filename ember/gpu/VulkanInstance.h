#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

#include "AppInfo.h"

namespace ember::gpu {

    class VulkanInstance {
    public:
        friend class GPUDevice;

        static inline std::shared_ptr<const VulkanInstance> create(const AppInfo& app_info) {
            return std::shared_ptr<const VulkanInstance>(new VulkanInstance(app_info));
        }
        ~VulkanInstance();

        vk::SurfaceKHR create_window_surface(SDL_Window* window) const;
        void destroy_window_surface(vk::SurfaceKHR surface) const;

    private:
        vk::Instance m_instance;
        vk::DebugUtilsMessengerEXT m_debug_messenger;

        VulkanInstance(const AppInfo& app_info);
        inline vk::Instance instance() const { return m_instance; }
    };

}
