#pragma once

#include <vector>
#include "Vulkan.h"

namespace ember::gpu {

    struct DescriptorSetChunk {
        std::vector<vk::DescriptorSet> sets;
        vk::DescriptorPool pool;
    };

}