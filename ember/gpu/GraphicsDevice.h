#pragma once

#include <memory>

#include "AppInfo.h"
#include "src/Vulkan.h"

namespace ember::gpu {

    class GraphicsDevice {
    public:
        static std::shared_ptr<const GraphicsDevice> create(const AppInfo& app_info) {
            return std::shared_ptr<const GraphicsDevice>(new GraphicsDevice(app_info));
        }
        ~GraphicsDevice();

        std::tuple<vk::Device, vk::Queue, vk::CommandPool> create_render_objects() const;

        inline const vk::PhysicalDeviceMemoryProperties& memory_properties() const {
            return m_memory_properties;
        }
        inline const uint32_t queue_family_index() const {
            return m_queue_family_index;
        }
    private:
        vk::Instance m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::PhysicalDeviceMemoryProperties m_memory_properties;
        uint32_t m_queue_family_index;

        GraphicsDevice(const AppInfo& app_info);
    };

}