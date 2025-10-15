#pragma once

#include "Vulkan.h"

namespace ember::gpu {

    struct Buffer {
        vk::DeviceSize size;
        vk::Buffer buffer;
        vk::DeviceMemory memory;
    };
    
}