#pragma once

#include <vector>

#include "Vulkan.h"

namespace ember::gpu {

    struct ShaderModule {
        vk::ShaderModule shader_module;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptor_set_layout_bindings;
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        std::vector<vk::PushConstantRange> push_constant_ranges;

        struct DescriptorPool {
            vk::DescriptorPool pool;
            uint32_t allocated_sets;
            uint32_t max_sets;
            uint32_t highwater_sets;
        };
        std::vector<DescriptorPool> descriptor_pools;
    };

}