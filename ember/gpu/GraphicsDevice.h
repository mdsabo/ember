#pragma once

#include <memory>

#include "AppInfo.h"
#include "Vulkan.h"

namespace ember::gpu {

    class GraphicsDevice {
    public:
        static std::shared_ptr<const GraphicsDevice> create(const AppInfo& app_info) {
            return std::shared_ptr<const GraphicsDevice>(new GraphicsDevice(app_info));
        }
        ~GraphicsDevice();

        std::pair<vk::Device, vk::Queue> create_device_and_queue() const;
        inline const vk::PhysicalDeviceMemoryProperties& memory_properties() const { return m_memory_properties; }
    private:
        vk::Instance m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::PhysicalDeviceMemoryProperties m_memory_properties;
        uint32_t m_queue_family_index;

        GraphicsDevice(const AppInfo& app_info);
    };

}