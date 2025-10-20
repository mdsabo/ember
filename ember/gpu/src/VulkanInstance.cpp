#include "VulkanInstance.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdexcept>

#include "ember/util/Log.h"

#define vkGetProcAddr(instance, fn) reinterpret_cast<PFN_##fn>(instance.getProcAddr(#fn));

// See https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples/CreateDebugUtilsMessenger/CreateDebugUtilsMessenger.cpp
static PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
static PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger
) {
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    VkAllocationCallbacks const* pAllocator
) {
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

namespace ember::gpu {

    namespace {
        void initialize_SDL() {
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                error(EMBER_GPU_LOG, "Failed to init SDL_INIT_VIDEO : {}", SDL_GetError());
                throw std::runtime_error("");
            }
            if (!SDL_Vulkan_LoadLibrary(nullptr)) {
                error(EMBER_GPU_LOG, "Failed to init SDL_INIT_VIDEO : {}", SDL_GetError());
                throw std::runtime_error("");
            }
        }

        std::vector<const char*> get_instance_layers(const EngineFeatures& features) {
            auto layers = std::vector<const char*>();

            if (features.vk_layer_api_dump) {
                layers.push_back("VK_LAYER_LUNARG_api_dump");
            }
            if (features.vk_layer_validation) {
                layers.push_back("VK_LAYER_KHRONOS_validation");
            }

            trace(EMBER_GPU_LOG, "Instance Layers ({})", layers.size());
            for (const auto layer : layers) {
                trace(EMBER_GPU_LOG, "    {}", layer);
            }

            return layers;
        }

        std::vector<const char*> get_instance_extensions(const EngineFeatures& features) {
            uint32_t sdl_instance_extension_count;
            auto sdl_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_instance_extension_count);

            auto extensions = std::vector<const char*>();
            extensions.reserve(sdl_instance_extension_count + 3);

            for (auto i = 0; i < sdl_instance_extension_count; i++) {
                extensions.push_back(sdl_instance_extensions[i]);
            }

            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#if defined(__APPLE__)
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            // Portability requires this extension as well
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

            trace(EMBER_GPU_LOG, "Instance Extensions ({})", extensions.size());
            for (const auto ext : extensions) {
                trace(EMBER_GPU_LOG, "    {}", ext);
            }

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
                .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
                .pApplicationInfo = &vk_app_info,
                .enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
                .ppEnabledLayerNames = instance_layers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
                .ppEnabledExtensionNames = instance_extensions.data(),
            };

            return vk::createInstance(instance_create_info);
        }
    }

    vk::Bool32 vulkan_debug_utils_callback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT types,
            const vk::DebugUtilsMessengerCallbackDataEXT* data,
            void* user_data
        ) {
            switch (severity) {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
                error(EMBER_GPU_LOG, "{}", data->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
                warn(EMBER_GPU_LOG, "{}", data->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
                info(EMBER_GPU_LOG, "{}", data->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
                trace(EMBER_GPU_LOG, "{}", data->pMessage);
                break;
            }

            // The callback returns a VkBool32, which is interpreted in a layer-specified manner.
            // The application should always return VK_FALSE. The VK_TRUE value is reserved for
            // use in layer development.
            return vk::False;
        }

        vk::DebugUtilsMessengerEXT create_debug_messenger(vk::Instance& instance) {
            const vk::DebugUtilsMessengerCreateInfoEXT create_info {
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
                .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                // Provided by VK_EXT_device_address_binding_report
                // | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
                .pfnUserCallback = vulkan_debug_utils_callback
            };

            pfnVkCreateDebugUtilsMessengerEXT = vkGetProcAddr(instance, vkCreateDebugUtilsMessengerEXT);
            pfnVkDestroyDebugUtilsMessengerEXT = vkGetProcAddr(instance, vkDestroyDebugUtilsMessengerEXT);
            return instance.createDebugUtilsMessengerEXT(create_info);
        }

    VulkanInstance::VulkanInstance(const AppInfo& app_info) {
        initialize_SDL();
        m_instance = create_instance(app_info);
        m_debug_messenger = create_debug_messenger(m_instance);
    }

    VulkanInstance::~VulkanInstance() {
        m_instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
        m_instance.destroy();
        SDL_Vulkan_UnloadLibrary();
    }

}