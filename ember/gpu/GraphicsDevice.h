#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "AppInfo.h"
#include "VulkanInstance.h"

namespace ember::gpu {

    class GraphicsDevice {
    public:
        static inline std::shared_ptr<const GraphicsDevice> create(
            std::shared_ptr<const VulkanInstance> instance,
            vk::SurfaceKHR surface = VK_NULL_HANDLE
        ) {
            return std::shared_ptr<const GraphicsDevice>(new GraphicsDevice(instance, surface));
        }
        ~GraphicsDevice();

        std::tuple<vk::Device, vk::Queue, vk::CommandPool> create_render_objects() const;

        vk::SwapchainCreateInfoKHR get_swapchain_create_info_for_surface(
            vk::SurfaceKHR surface,
            vk::Extent2D window_extent,
            vk::SwapchainKHR old_swapchain
        ) const;

        inline const vk::PhysicalDeviceMemoryProperties& memory_properties() const {
            return m_memory_properties;
        }
        inline const uint32_t queue_family_index() const {
            return m_queue_family_index;
        }
    private:
        std::shared_ptr<const VulkanInstance> m_instance;
        vk::PhysicalDevice m_physical_device;
        vk::PhysicalDeviceMemoryProperties m_memory_properties;
        uint32_t m_queue_family_index;

        GraphicsDevice(std::shared_ptr<const VulkanInstance> instance, vk::SurfaceKHR surface);
    };

}