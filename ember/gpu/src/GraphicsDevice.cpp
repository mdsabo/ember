#include "GraphicsDevice.h"

#include <algorithm>
#include <array>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <string_view>
#include <vector>

#include "ember/util/Log.h"
#include "Util.h"
#include "Vulkan.h"

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

            trace(EMBER_GPU_LOG, "GraphicsDevice : Instance Layers ({})", layers.size());
            for (const auto layer : layers) {
                trace(EMBER_GPU_LOG, "|   {}", layer);
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

            trace(EMBER_GPU_LOG, "GraphicsDevice : Instance Extensions ({})", extensions.size());
            for (const auto ext : extensions) {
                trace(EMBER_GPU_LOG, "|   {}", ext);
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

        void log_physical_device(vk::PhysicalDevice device) {
            auto properties = device.getProperties();
            info(EMBER_GPU_LOG, "GraphicsDevice : Physical Device \"{}\"", properties.deviceName.data());
            info(EMBER_GPU_LOG, "|   Device Type: {}", to_string(properties.deviceType));
            info(EMBER_GPU_LOG, "|   Device ID: {}", properties.deviceID);
            info(EMBER_GPU_LOG, "|   API version: {}", properties.apiVersion);
            info(EMBER_GPU_LOG, "|   Driver Version: {}", properties.driverVersion);
            info(EMBER_GPU_LOG, "|   Vendor ID: {}", properties.vendorID);

            info(EMBER_GPU_LOG, "|   Memory Properties");
            auto memory_properties = device.getMemoryProperties();
            for (auto i = 0; i < memory_properties.memoryTypeCount; i++) {
                const auto& memtype = memory_properties.memoryTypes[i];
                info(EMBER_GPU_LOG, "|   |   Memory Type[{}]: Heap[{}] {}", i, memtype.heapIndex, to_string(memtype.propertyFlags));
            }
            for (auto i = 0; i < memory_properties.memoryHeapCount; i++) {
                const auto& heap = memory_properties.memoryHeaps[i];
                info(EMBER_GPU_LOG, "|   |   Memory Heap[{}]: {} {}", i, to_string(heap.flags), heap.size);
            }

            info(EMBER_GPU_LOG, "|   Device Features");
            auto features = device.getFeatures();
            info(EMBER_GPU_LOG, "|   |   robustBufferAccess: {}", features.robustBufferAccess);
            info(EMBER_GPU_LOG, "|   |   fullDrawIndexUint32: {}", features.fullDrawIndexUint32);
            info(EMBER_GPU_LOG, "|   |   imageCubeArray: {}", features.imageCubeArray);
            info(EMBER_GPU_LOG, "|   |   independentBlend: {}", features.independentBlend);
            info(EMBER_GPU_LOG, "|   |   geometryShader: {}", features.geometryShader);
            info(EMBER_GPU_LOG, "|   |   tessellationShader: {}", features.tessellationShader);
            info(EMBER_GPU_LOG, "|   |   sampleRateShading: {}", features.sampleRateShading);
            info(EMBER_GPU_LOG, "|   |   dualSrcBlend: {}", features.dualSrcBlend);
            info(EMBER_GPU_LOG, "|   |   logicOp: {}", features.logicOp);
            info(EMBER_GPU_LOG, "|   |   multiDrawIndirect: {}", features.multiDrawIndirect);
            info(EMBER_GPU_LOG, "|   |   drawIndirectFirstInstance: {}", features.drawIndirectFirstInstance);
            info(EMBER_GPU_LOG, "|   |   depthClamp: {}", features.depthClamp);
            info(EMBER_GPU_LOG, "|   |   depthBiasClamp: {}", features.depthBiasClamp);
            info(EMBER_GPU_LOG, "|   |   fillModeNonSolid: {}", features.fillModeNonSolid);
            info(EMBER_GPU_LOG, "|   |   depthBounds: {}", features.depthBounds);
            info(EMBER_GPU_LOG, "|   |   wideLines: {}", features.wideLines);
            info(EMBER_GPU_LOG, "|   |   largePoints: {}", features.largePoints);
            info(EMBER_GPU_LOG, "|   |   alphaToOne: {}", features.alphaToOne);
            info(EMBER_GPU_LOG, "|   |   multiViewport: {}", features.multiViewport);
            info(EMBER_GPU_LOG, "|   |   samplerAnisotropy: {}", features.samplerAnisotropy);
            info(EMBER_GPU_LOG, "|   |   textureCompressionETC2: {}", features.textureCompressionETC2);
            info(EMBER_GPU_LOG, "|   |   textureCompressionASTC_LDR: {}", features.textureCompressionASTC_LDR);
            info(EMBER_GPU_LOG, "|   |   textureCompressionBC: {}", features.textureCompressionBC);
            info(EMBER_GPU_LOG, "|   |   occlusionQueryPrecise: {}", features.occlusionQueryPrecise);
            info(EMBER_GPU_LOG, "|   |   pipelineStatisticsQuery: {}", features.pipelineStatisticsQuery);
            info(EMBER_GPU_LOG, "|   |   vertexPipelineStoresAndAtomics: {}", features.vertexPipelineStoresAndAtomics);
            info(EMBER_GPU_LOG, "|   |   fragmentStoresAndAtomics: {}", features.fragmentStoresAndAtomics);
            info(EMBER_GPU_LOG, "|   |   shaderTessellationAndGeometryPointSize: {}", features.shaderTessellationAndGeometryPointSize);
            info(EMBER_GPU_LOG, "|   |   shaderImageGatherExtended: {}", features.shaderImageGatherExtended);
            info(EMBER_GPU_LOG, "|   |   shaderStorageImageExtendedFormats: {}", features.shaderStorageImageExtendedFormats);
            info(EMBER_GPU_LOG, "|   |   shaderStorageImageMultisample: {}", features.shaderStorageImageMultisample);
            info(EMBER_GPU_LOG, "|   |   shaderStorageImageReadWithoutFormat: {}", features.shaderStorageImageReadWithoutFormat);
            info(EMBER_GPU_LOG, "|   |   shaderStorageImageWriteWithoutFormat: {}", features.shaderStorageImageWriteWithoutFormat);
            info(EMBER_GPU_LOG, "|   |   shaderUniformBufferArrayDynamicIndexing: {}", features.shaderUniformBufferArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "|   |   shaderSampledImageArrayDynamicIndexing: {}", features.shaderSampledImageArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "|   |   shaderStorageBufferArrayDynamicIndexing: {}", features.shaderStorageBufferArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "|   |   shaderStorageImageArrayDynamicIndexing: {}", features.shaderStorageImageArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "|   |   shaderClipDistance: {}", features.shaderClipDistance);
            info(EMBER_GPU_LOG, "|   |   shaderCullDistance: {}", features.shaderCullDistance);
            info(EMBER_GPU_LOG, "|   |   shaderFloat64: {}", features.shaderFloat64);
            info(EMBER_GPU_LOG, "|   |   shaderInt64: {}", features.shaderInt64);
            info(EMBER_GPU_LOG, "|   |   shaderInt16: {}", features.shaderInt16);
            info(EMBER_GPU_LOG, "|   |   shaderResourceResidency: {}", features.shaderResourceResidency);
            info(EMBER_GPU_LOG, "|   |   shaderResourceMinLod: {}", features.shaderResourceMinLod);
            info(EMBER_GPU_LOG, "|   |   sparseBinding: {}", features.sparseBinding);
            info(EMBER_GPU_LOG, "|   |   sparseResidencyBuffer: {}", features.sparseResidencyBuffer);
            info(EMBER_GPU_LOG, "|   |   sparseResidencyImage2D: {}", features.sparseResidencyImage2D);
            info(EMBER_GPU_LOG, "|   |   sparseResidencyImage3D: {}", features.sparseResidencyImage3D);
            info(EMBER_GPU_LOG, "|   |   sparseResidency2Samples: {}", features.sparseResidency2Samples);
            info(EMBER_GPU_LOG, "|   |   sparseResidency4Samples: {}", features.sparseResidency4Samples);
            info(EMBER_GPU_LOG, "|   |   sparseResidency8Samples: {}", features.sparseResidency8Samples);
            info(EMBER_GPU_LOG, "|   |   sparseResidency16Samples: {}", features.sparseResidency16Samples);
            info(EMBER_GPU_LOG, "|   |   sparseResidencyAliased: {}", features.sparseResidencyAliased);
            info(EMBER_GPU_LOG, "|   |   variableMultisampleRate: {}", features.variableMultisampleRate);
            info(EMBER_GPU_LOG, "|   |   inheritedQueries: {}", features.inheritedQueries);

            info(EMBER_GPU_LOG, "|   Queue Familes");
            auto queue_familes = device.getQueueFamilyProperties();
            for (auto i = 0; i < queue_familes.size(); i++) {
                const auto& qf = queue_familes[i];
                info(EMBER_GPU_LOG, "|   |   [{}] = {} Queues, {}", i, qf.queueCount, to_string(qf.queueFlags));
            }
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
    } // namespace

    GraphicsDevice::GraphicsDevice(const AppInfo& app_info) {
        initialize_SDL();
        m_instance = create_instance(app_info);
        m_debug_messenger = create_debug_messenger(m_instance);

        m_physical_device = select_physical_device(m_instance);
        log_physical_device(m_physical_device);

        m_memory_properties = m_physical_device.getMemoryProperties();
        m_queue_family_index = find_queue_family(m_physical_device);
        trace(EMBER_GPU_LOG, "Graphics Device : Using queue family {}", m_queue_family_index);
    }

    GraphicsDevice::~GraphicsDevice() {
        m_instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
        m_instance.destroy();
        SDL_Vulkan_UnloadLibrary();
    }

    std::tuple<vk::Device, vk::Queue, vk::CommandPool> GraphicsDevice::create_render_objects() const {
        constexpr std::array QUEUE_PRIORTIES = { 1.0f };
        const vk::DeviceQueueCreateInfo queue_create_info{
            .queueFamilyIndex = m_queue_family_index,
            .queueCount = 1,
            .pQueuePriorities = QUEUE_PRIORTIES.data()
        };

        std::vector<const char*> extensions(REQUIRED_DEVICE_EXTENSIONS.size());
        std::copy(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end(), extensions.begin());
#if defined(__APPLE__)
        extensions.push_back("VK_KHR_portability_subset");
#endif
            trace(EMBER_GPU_LOG, "GraphicsDevice : Device Extensions ({})", extensions.size());
            for (const auto ext : extensions) {
                trace(EMBER_GPU_LOG, "|   {}", ext);
            }

        const vk::DeviceCreateInfo device_create_info{
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = &REQUIRED_DEVICE_FEATURES,
        };

        auto device = m_physical_device.createDevice(device_create_info);
        auto queue = device.getQueue(m_queue_family_index, 0);
        auto command_pool = device.createCommandPool(vk::CommandPoolCreateInfo {
            .queueFamilyIndex = m_queue_family_index
        });
        return std::make_tuple(device, queue, command_pool);
    }

}