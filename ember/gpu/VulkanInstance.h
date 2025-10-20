#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

#include "AppInfo.h"

namespace ember::gpu {

    class VulkanInstance {
    public:
        friend class GraphicsDevice;
        friend class Window;

        static inline std::shared_ptr<const VulkanInstance> create(const AppInfo& app_info) {
            return std::shared_ptr<const VulkanInstance>(new VulkanInstance(app_info));
        }
        ~VulkanInstance();

    private:
        vk::Instance m_instance;
        vk::DebugUtilsMessengerEXT m_debug_messenger;

        inline vk::Instance instance() const { return m_instance; }

        VulkanInstance(const AppInfo& app_info);
    };

}
