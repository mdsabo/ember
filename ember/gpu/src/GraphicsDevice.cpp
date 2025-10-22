#include "GraphicsDevice.h"

#include <algorithm>
#include <array>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <string_view>
#include <vector>

#include "ember/util/Log.h"
#include "Util.h"

namespace ember::gpu {

    namespace {
        bool device_supports_extension(
            std::string_view extension,
            const std::vector<vk::ExtensionProperties> device_extensions
        ) {
            for (const auto ext : device_extensions) {
                if (extension == ext.extensionName) return true;
            }
            return false;
        }

        constexpr std::array REQUIRED_DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
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

        uint32_t find_suitable_queue_family(const vk::PhysicalDevice& physical_device, vk::SurfaceKHR surface) {
            constexpr auto QUEUE_FLAGS = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;

            auto queue_family_properties = physical_device.getQueueFamilyProperties();
            for (auto i = 0; i < queue_family_properties.size(); i++) {
                const auto& properties = queue_family_properties[i];

                bool has_flags = vkFlagContains(properties.queueFlags, QUEUE_FLAGS);
                bool supports_surface = (surface == VK_NULL_HANDLE)
                    || physical_device.getSurfaceSupportKHR(i, surface);

                if (has_flags && supports_surface) return i;
            }

            return -1;
        }

        std::pair<vk::PhysicalDevice, uint32_t> select_physical_device_and_queue_family(
            vk::Instance instance,
            vk::SurfaceKHR surface
        ) {
            auto all_devices = instance.enumeratePhysicalDevices();
            std::vector<std::pair<vk::PhysicalDevice, uint32_t>> supported_devices;

            for (const auto device : all_devices) {
                const auto properties = device.getProperties();
                [[maybe_unused]] auto device_name = properties.deviceName.data();
                trace(EMBER_GPU_LOG, "Checking suitability of \"{}\"", device_name);

                if (!device_supports_required_extensions(device)) {
                    trace(EMBER_GPU_LOG, "    Does not support required extensions");
                    continue;
                }

                if (!device_supports_required_features(device)) {
                    trace(EMBER_GPU_LOG, "    Does not support required features");
                    continue;
                }

                auto queue_family_index = find_suitable_queue_family(device, surface);
                if (queue_family_index == -1) {
                    trace(EMBER_GPU_LOG, "    Does not have any suitable queue families");
                    continue;
                }

                trace(EMBER_GPU_LOG, "    Supports all extensions, features and surface");

                const auto device_and_qfi = std::make_pair(device, queue_family_index);
                if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                    trace(EMBER_GPU_LOG, "    Device is Discrete GPU -> Choosing \"{}\"", device_name);
                    return device_and_qfi;
                }

                supported_devices.push_back(device_and_qfi);
            }

            trace(EMBER_GPU_LOG, "Graphics Device : No discrete GPUs found falling back to first suitable device");
            if (supported_devices.empty()) {
                throw std::runtime_error("No suitable Vulkan devices found!");
            }
            return supported_devices.front();
        }

        void log_physical_device(vk::PhysicalDevice device) {
            auto properties = device.getProperties();
            info(EMBER_GPU_LOG, "Physical Device \"{}\"", properties.deviceName.data());
            info(EMBER_GPU_LOG, "    Device Type: {}", to_string(properties.deviceType));
            info(EMBER_GPU_LOG, "    Device ID: {}", properties.deviceID);
            info(EMBER_GPU_LOG, "    API version: {}", properties.apiVersion);
            info(EMBER_GPU_LOG, "    Driver Version: {}", properties.driverVersion);
            info(EMBER_GPU_LOG, "    Vendor ID: {}", properties.vendorID);

            info(EMBER_GPU_LOG, "    Memory Properties");
            auto memory_properties = device.getMemoryProperties();
            for (auto i = 0; i < memory_properties.memoryTypeCount; i++) {
                const auto& memtype = memory_properties.memoryTypes[i];
                info(EMBER_GPU_LOG, "        Memory Type[{}]: Heap[{}] {}", i, memtype.heapIndex, to_string(memtype.propertyFlags));
            }
            for (auto i = 0; i < memory_properties.memoryHeapCount; i++) {
                const auto& heap = memory_properties.memoryHeaps[i];
                info(EMBER_GPU_LOG, "        Memory Heap[{}]: {} {}", i, to_string(heap.flags), heap.size);
            }

            info(EMBER_GPU_LOG, "    Device Features");
            auto features = device.getFeatures();
            info(EMBER_GPU_LOG, "        robustBufferAccess: {}", features.robustBufferAccess);
            info(EMBER_GPU_LOG, "        fullDrawIndexUint32: {}", features.fullDrawIndexUint32);
            info(EMBER_GPU_LOG, "        imageCubeArray: {}", features.imageCubeArray);
            info(EMBER_GPU_LOG, "        independentBlend: {}", features.independentBlend);
            info(EMBER_GPU_LOG, "        geometryShader: {}", features.geometryShader);
            info(EMBER_GPU_LOG, "        tessellationShader: {}", features.tessellationShader);
            info(EMBER_GPU_LOG, "        sampleRateShading: {}", features.sampleRateShading);
            info(EMBER_GPU_LOG, "        dualSrcBlend: {}", features.dualSrcBlend);
            info(EMBER_GPU_LOG, "        logicOp: {}", features.logicOp);
            info(EMBER_GPU_LOG, "        multiDrawIndirect: {}", features.multiDrawIndirect);
            info(EMBER_GPU_LOG, "        drawIndirectFirstInstance: {}", features.drawIndirectFirstInstance);
            info(EMBER_GPU_LOG, "        depthClamp: {}", features.depthClamp);
            info(EMBER_GPU_LOG, "        depthBiasClamp: {}", features.depthBiasClamp);
            info(EMBER_GPU_LOG, "        fillModeNonSolid: {}", features.fillModeNonSolid);
            info(EMBER_GPU_LOG, "        depthBounds: {}", features.depthBounds);
            info(EMBER_GPU_LOG, "        wideLines: {}", features.wideLines);
            info(EMBER_GPU_LOG, "        largePoints: {}", features.largePoints);
            info(EMBER_GPU_LOG, "        alphaToOne: {}", features.alphaToOne);
            info(EMBER_GPU_LOG, "        multiViewport: {}", features.multiViewport);
            info(EMBER_GPU_LOG, "        samplerAnisotropy: {}", features.samplerAnisotropy);
            info(EMBER_GPU_LOG, "        textureCompressionETC2: {}", features.textureCompressionETC2);
            info(EMBER_GPU_LOG, "        textureCompressionASTC_LDR: {}", features.textureCompressionASTC_LDR);
            info(EMBER_GPU_LOG, "        textureCompressionBC: {}", features.textureCompressionBC);
            info(EMBER_GPU_LOG, "        occlusionQueryPrecise: {}", features.occlusionQueryPrecise);
            info(EMBER_GPU_LOG, "        pipelineStatisticsQuery: {}", features.pipelineStatisticsQuery);
            info(EMBER_GPU_LOG, "        vertexPipelineStoresAndAtomics: {}", features.vertexPipelineStoresAndAtomics);
            info(EMBER_GPU_LOG, "        fragmentStoresAndAtomics: {}", features.fragmentStoresAndAtomics);
            info(EMBER_GPU_LOG, "        shaderTessellationAndGeometryPointSize: {}", features.shaderTessellationAndGeometryPointSize);
            info(EMBER_GPU_LOG, "        shaderImageGatherExtended: {}", features.shaderImageGatherExtended);
            info(EMBER_GPU_LOG, "        shaderStorageImageExtendedFormats: {}", features.shaderStorageImageExtendedFormats);
            info(EMBER_GPU_LOG, "        shaderStorageImageMultisample: {}", features.shaderStorageImageMultisample);
            info(EMBER_GPU_LOG, "        shaderStorageImageReadWithoutFormat: {}", features.shaderStorageImageReadWithoutFormat);
            info(EMBER_GPU_LOG, "        shaderStorageImageWriteWithoutFormat: {}", features.shaderStorageImageWriteWithoutFormat);
            info(EMBER_GPU_LOG, "        shaderUniformBufferArrayDynamicIndexing: {}", features.shaderUniformBufferArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "        shaderSampledImageArrayDynamicIndexing: {}", features.shaderSampledImageArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "        shaderStorageBufferArrayDynamicIndexing: {}", features.shaderStorageBufferArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "        shaderStorageImageArrayDynamicIndexing: {}", features.shaderStorageImageArrayDynamicIndexing);
            info(EMBER_GPU_LOG, "        shaderClipDistance: {}", features.shaderClipDistance);
            info(EMBER_GPU_LOG, "        shaderCullDistance: {}", features.shaderCullDistance);
            info(EMBER_GPU_LOG, "        shaderFloat64: {}", features.shaderFloat64);
            info(EMBER_GPU_LOG, "        shaderInt64: {}", features.shaderInt64);
            info(EMBER_GPU_LOG, "        shaderInt16: {}", features.shaderInt16);
            info(EMBER_GPU_LOG, "        shaderResourceResidency: {}", features.shaderResourceResidency);
            info(EMBER_GPU_LOG, "        shaderResourceMinLod: {}", features.shaderResourceMinLod);
            info(EMBER_GPU_LOG, "        sparseBinding: {}", features.sparseBinding);
            info(EMBER_GPU_LOG, "        sparseResidencyBuffer: {}", features.sparseResidencyBuffer);
            info(EMBER_GPU_LOG, "        sparseResidencyImage2D: {}", features.sparseResidencyImage2D);
            info(EMBER_GPU_LOG, "        sparseResidencyImage3D: {}", features.sparseResidencyImage3D);
            info(EMBER_GPU_LOG, "        sparseResidency2Samples: {}", features.sparseResidency2Samples);
            info(EMBER_GPU_LOG, "        sparseResidency4Samples: {}", features.sparseResidency4Samples);
            info(EMBER_GPU_LOG, "        sparseResidency8Samples: {}", features.sparseResidency8Samples);
            info(EMBER_GPU_LOG, "        sparseResidency16Samples: {}", features.sparseResidency16Samples);
            info(EMBER_GPU_LOG, "        sparseResidencyAliased: {}", features.sparseResidencyAliased);
            info(EMBER_GPU_LOG, "        variableMultisampleRate: {}", features.variableMultisampleRate);
            info(EMBER_GPU_LOG, "        inheritedQueries: {}", features.inheritedQueries);

            info(EMBER_GPU_LOG, "    Queue Familes");
            auto queue_familes = device.getQueueFamilyProperties();
            for (auto i = 0; i < queue_familes.size(); i++) {
                const auto& qf = queue_familes[i];
                info(EMBER_GPU_LOG, "        [{}] = {} Queues, {}", i, qf.queueCount, to_string(qf.queueFlags));
            }
        }
    } // namespace

    GraphicsDevice::GraphicsDevice(std::shared_ptr<const VulkanInstance> instance, vk::SurfaceKHR surface):
        m_instance(instance)
    {
        auto [device, qfi] = select_physical_device_and_queue_family(
            m_instance->instance(), surface
        );
        log_physical_device(device);
        trace(EMBER_GPU_LOG, "Graphics Device : Using queue family {}", qfi);

        m_physical_device = device;
        m_queue_family_index = qfi;
        m_memory_properties = m_physical_device.getMemoryProperties();
    }

    GraphicsDevice::~GraphicsDevice() { }

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
            trace(EMBER_GPU_LOG, "Device Extensions ({})", extensions.size());
            for (const auto ext : extensions) {
                trace(EMBER_GPU_LOG, "    {}", ext);
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

    namespace {
        uint32_t surface_preferred_image_count(const vk::SurfaceCapabilitiesKHR& capabilities) {
            auto preferred_image_count = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount == 0) {
                return preferred_image_count;
            } else {
                return std::min(preferred_image_count, capabilities.maxImageCount);
            }
        }

        vk::SurfaceFormatKHR surface_preferred_format(const std::vector<vk::SurfaceFormatKHR>& formats) {
            return formats.front(); // FIXME : how do you select the best format anyway?
        }

        vk::Extent2D surface_preferred_extent(
            const vk::SurfaceCapabilitiesKHR& capabilities,
            vk::Extent2D window_extent
        ) {
            if (capabilities.currentExtent.width == 0xFFFFFFFF) {
                return window_extent;
            } else {
                return capabilities.currentExtent;
            }
        }

        vk::SurfaceTransformFlagBitsKHR surface_preferred_transform(
            const vk::SurfaceCapabilitiesKHR& capabilities
        ) {
            if (vkFlagContains(capabilities.currentTransform, vk::SurfaceTransformFlagBitsKHR::eIdentity)) {
                return vk::SurfaceTransformFlagBitsKHR::eIdentity;
            } else {
                return capabilities.currentTransform;
            }
        }

        vk::PresentModeKHR surface_preferred_present_mode(
            const std::vector<vk::PresentModeKHR>& present_modes
        ) {
            if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end()) {
                return vk::PresentModeKHR::eMailbox;
            } else {
                return vk::PresentModeKHR::eFifo;
            }
        }
    }

    vk::SwapchainCreateInfoKHR GraphicsDevice::get_swapchain_create_info_for_surface(
        vk::SurfaceKHR surface,
        vk::Extent2D window_extent,
        vk::SwapchainKHR old_swapchain
    ) const {
        auto capabilities = m_physical_device.getSurfaceCapabilitiesKHR(surface);
        auto formats = m_physical_device.getSurfaceFormatsKHR(surface);
        auto preferred_format = surface_preferred_format(formats);
        auto present_modes = m_physical_device.getSurfacePresentModesKHR(surface);

        return vk::SwapchainCreateInfoKHR {
            .surface = surface,
            .minImageCount = surface_preferred_image_count(capabilities),
            .imageFormat = preferred_format.format,
            .imageColorSpace = preferred_format.colorSpace,
            .imageExtent = surface_preferred_extent(capabilities, window_extent),
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &m_queue_family_index,
            .preTransform = surface_preferred_transform(capabilities),
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = surface_preferred_present_mode(present_modes),
            .clipped = vk::True,
            .oldSwapchain = old_swapchain
        };
    }

}