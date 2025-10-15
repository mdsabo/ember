#include "GraphicsDevice.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

#include "Util.h"
#include "Vulkan.h"

namespace ember::gpu {

    namespace {
        std::vector<const char*> get_instance_layers(const EngineFeatures& features) {
            auto layers = std::vector<const char*>();

            if (features.vk_layer_api_dump) {
                layers.push_back("VK_LAYER_LUNARG_api_dump");
            }
            if (features.vk_layer_validation) {
                layers.push_back("VK_LAYER_KHRONOS_validation");
            }

            return layers;
        }

        std::vector<const char*> get_instance_extensions(const EngineFeatures& features) {
            auto extensions = std::vector<const char*>();

            if (features.vk_extension_debug_utils) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

#if defined(__APPLE__)
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            // Portability requires this extension as well
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

            return extensions;
        }

        vk::Instance create_instance(const AppInfo& app_info) {
            const vk::ApplicationInfo vk_app_info {
                .pApplicationName = app_info.name,
                .applicationVersion = app_info.version,
                .pEngineName = "Ember",
                .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
                .apiVersion = VK_API_VERSION_1_4
            };

            const auto instance_layers = get_instance_layers(app_info.features);
            const auto instance_extensions = get_instance_extensions(app_info.features);



            const vk::InstanceCreateInfo instance_create_info {
#if defined(__APPLE__)
                .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif
                .pApplicationInfo = &vk_app_info,
                .enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
                .ppEnabledLayerNames = instance_layers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
                .ppEnabledExtensionNames = instance_extensions.data(),
            };

            return vk::createInstance(instance_create_info);
        }

        bool device_supports_extension(
            std::string_view extension,
            const std::vector<vk::ExtensionProperties> device_extensions
        ) {
            for (const auto ext : device_extensions) {
                if (extension == ext.extensionName) return true;
            }
            return false;
        }

        constexpr std::array<const char*, 0> REQUIRED_DEVICE_EXTENSIONS = {};
        bool device_supports_required_extensions(const vk::PhysicalDevice& device) {
            auto device_extensions = device.enumerateDeviceExtensionProperties();
            for (const auto ext : REQUIRED_DEVICE_EXTENSIONS) {
                if (!device_supports_extension(ext, device_extensions)) return false;
            }
            return true;
        }

        constexpr vk::PhysicalDeviceFeatures REQUIRED_DEVICE_FEATURES = {};
        bool device_supports_required_features(const vk::PhysicalDevice& device) {
            //
            return true;
        }

        bool device_supports_surface(const vk::PhysicalDevice& device) {
            return true;
        }

        vk::PhysicalDevice select_physical_device(vk::Instance& instance) {
            auto devices = instance.enumeratePhysicalDevices();

            auto end = std::remove_if(devices.begin(), devices.end(), [](const vk::PhysicalDevice& device) {
                return !device_supports_required_extensions(device)
                || !device_supports_required_features(device)
                || !device_supports_surface(device);
            });

            auto device = std::find_if(devices.begin(), end, [](const vk::PhysicalDevice& device) {
                return device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
            });
            if (device != end) return *device;

            device = std::find_if(devices.begin(), end, [](const vk::PhysicalDevice& device) {
                return device.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
            });
            if (device != end) return *device;

            return devices.front();
        }

        uint32_t find_queue_family(const vk::PhysicalDevice& physical_device) {
            constexpr auto QUEUE_FLAGS = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
            auto queue_family_properties = physical_device.getQueueFamilyProperties();
            for (auto i = 0; i < queue_family_properties.size(); i++) {
                const auto& properties = queue_family_properties[i];
                if (vkFlagContains(properties.queueFlags, QUEUE_FLAGS)) return i;
            }
            throw std::runtime_error("No suitable queue family found!");
        }
    } // namespace

    GraphicsDevice::GraphicsDevice(const AppInfo& app_info) {
        m_instance = create_instance(app_info);
        m_physical_device = select_physical_device(m_instance);
        m_memory_properties = m_physical_device.getMemoryProperties();
        m_queue_family_index = find_queue_family(m_physical_device);
    }

    GraphicsDevice::~GraphicsDevice() {
        m_instance.destroy();
    }

    std::pair<vk::Device, vk::Queue> GraphicsDevice::create_device_and_queue() const {
        constexpr std::array QUEUE_PRIORTIES = { 1.0f };
        const vk::DeviceQueueCreateInfo queue_create_info{
            .queueFamilyIndex = m_queue_family_index,
            .queueCount = 1,
            .pQueuePriorities = QUEUE_PRIORTIES.data()
        };

        std::vector<const char*> extensions(REQUIRED_DEVICE_EXTENSIONS.size());
        std::copy(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end(), extensions.begin());
#if defined(__APPLE__)
        extensions.push_back(VK_KHR_PORTABILITY_SUBSET_NAME);
#endif

        const vk::DeviceCreateInfo device_create_info{
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = &REQUIRED_DEVICE_FEATURES,
        };

        auto device = m_physical_device.createDevice(device_create_info);
        auto queue = device.getQueue(m_queue_family_index, 0);
        return std::make_pair(device, queue);
    }

}