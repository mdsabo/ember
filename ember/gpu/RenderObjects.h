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

    struct ShaderModule {
        vk::ShaderModule module;
        std::unique_ptr<ShaderReflection> reflection = nullptr;
    };

    constexpr auto DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL = 32768;
    constexpr auto MAX_DESCRIPTOR_SETS = 4;
    template<typename T>
    using DescriptorSetArray = std::array<T, MAX_DESCRIPTOR_SETS>;

    struct DescriptorSetBlueprint {
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        vk::DescriptorSetLayout layout;

        vk::DescriptorPool pool;
        uint32_t allocated_sets = 0;
        uint32_t max_sets = DEFAULT_MAX_DESCRIPTOR_SETS_PER_POOL;
        uint32_t highwater_sets = 0;

        inline vk::DescriptorType descriptor_type(uint32_t binding) const {
            return vk::DescriptorType(layout_bindings[binding].descriptorType);
        }
    };

    struct Swapchain {
        vk::SwapchainKHR swapchain;
        vk::Format format;
        vk::Extent2D extent;
        struct RenderTarget {
            Image* image;
            Image* depth_image;
            vk::Semaphore render_complete; // Semaphore is signaled when corresponding image is ready to present
        };
        std::vector<RenderTarget> render_targets;
    };
}