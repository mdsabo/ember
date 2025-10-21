#pragma once

#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ShaderReflection.h"

namespace ember::gpu {

    struct Buffer {
        vk::DeviceSize size;
        vk::Buffer buffer;
        vk::DeviceMemory memory;
    };

    struct Image {
        vk::Extent3D extent;
        vk::ImageLayout layout;
        vk::Image image;
        vk::ImageView view;
        vk::DeviceMemory memory;
    };

    struct Pipeline {
        vk::PipelineBindPoint bind_point;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

    constexpr auto DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL = 32768;

    struct DescriptorPool {
        vk::DescriptorPool pool;
        uint32_t allocated_sets = 0;
        uint32_t max_sets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL;
        uint32_t highwater_sets = 0;
    };

    struct ShaderModule2 {
        vk::ShaderModule module;
        ShaderReflection reflection;
    };

    constexpr auto MAX_DESCRIPTOR_SETS = 4;
    template<typename T>
    using DescriptorSetArray = std::array<T, MAX_DESCRIPTOR_SETS>;
    constexpr auto MAX_DESCRIPTOR_BINDINGS_PER_SET = 16;
    template<typename T>
    using DescriptorBindingArray = DescriptorSetArray<std::array<T, MAX_DESCRIPTOR_BINDINGS_PER_SET>>;

    struct DescriptorSetAllocator {
        DescriptorSetArray<std::vector<vk::DescriptorSetLayoutBinding>> layout_bindings;
        DescriptorSetArray<vk::DescriptorSetLayout> layouts;
        DescriptorSetArray<DescriptorPool> pools;
    };

    struct ShaderModule {
        vk::ShaderStageFlagBits stage;
        vk::ShaderModule shader_module;
        uint32_t descriptor_set_count;
        DescriptorSetArray<size_t> descriptor_binding_counts;
        DescriptorSetArray<vk::DescriptorSetLayout> descriptor_set_layouts;
        DescriptorSetArray<DescriptorPool> descriptor_pools;
        DescriptorBindingArray<vk::DescriptorType> descriptor_types;
        std::vector<vk::PushConstantRange> push_constant_ranges;
    };

    struct DescriptorSets {
        ShaderModule* shader_module;
        uint32_t set_index;
        std::vector<vk::DescriptorSet> descriptor_sets;
    };
}