#pragma once

#include <memory>

#include "AppInfo.h"
#include "Vulkan.h"

namespace ember::gpu {

    class Device {
    public:
        static std::shared_ptr<Device> create(const AppInfo& app_info) {
            return std::shared_ptr<Device>(new Device(app_info));
        }
        ~Device();

    private:
        vk::Instance m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::PhysicalDeviceMemoryProperties m_memory_properties;
        uint32_t m_queue_family_index;

        Device(const AppInfo& app_info);
    };

}