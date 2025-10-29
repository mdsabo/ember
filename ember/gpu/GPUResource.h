#pragma once

#include "GPUDevice.h"
#include "GPUInterface.h"
#include "VulkanInstance.h"

namespace ember::gpu {

    struct GPUResource {
        static std::shared_ptr<const gpu::VulkanInstance> vulkan_instance;
        std::shared_ptr<const gpu::GPUDevice> gpu_device;
        std::unique_ptr<const gpu::GPUInterface> gpu_interface;
    };

}