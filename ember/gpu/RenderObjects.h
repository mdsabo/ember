#pragma once

#include <vector>
#include "src/Vulkan.h"

namespace ember::gpu {

    class Buffer {
        friend class Renderer;

        vk::DeviceSize size;
        vk::Buffer buffer;
        vk::DeviceMemory memory;

        Buffer() = default;
        Buffer(vk::DeviceSize size, vk::Buffer buffer, vk::DeviceMemory memory):
            size(size), buffer(buffer), memory(memory)
        { }
    };

    class DescriptorSetChunk {
        friend class Renderer;
        friend class CommandRecorder;

        std::vector<vk::DescriptorSet> sets;
        uint32_t set_index;

        DescriptorSetChunk() = default;
        DescriptorSetChunk(const std::vector<vk::DescriptorSet>& sets, uint32_t set_index):
            sets(sets), set_index(set_index)
        { }
    };

    class Pipeline {
        friend class Renderer;
        friend class CommandRecorder;

        vk::PipelineBindPoint bind_point;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;

        Pipeline() = default;
        Pipeline(vk::PipelineBindPoint bind_point, vk::PipelineLayout layout, vk::Pipeline pipeline):
            bind_point(bind_point), layout(layout), pipeline(pipeline)
        { }
    };

    struct ShaderModule {
        friend class Renderer;

        vk::ShaderModule shader_module;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptor_set_layout_bindings;

        struct DescriptorSetAllocationInfo {
            vk::DescriptorSetLayout layout;
            vk::DescriptorPool pool;
            uint32_t allocated_sets;
            uint32_t max_sets;
            uint32_t highwater_sets;
        };
        std::vector<DescriptorSetAllocationInfo> descriptor_set_allocation_infos;

        std::vector<vk::PushConstantRange> push_constant_ranges;

        ShaderModule() = default;
        ShaderModule(
            vk::ShaderModule shader_module,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& descriptor_set_layout_bindings,
            const std::vector<DescriptorSetAllocationInfo>& descriptor_set_allocation_infos,
            const std::vector<vk::PushConstantRange>& push_constant_ranges
        ):
            shader_module(shader_module), descriptor_set_layout_bindings(descriptor_set_layout_bindings),
            descriptor_set_allocation_infos(descriptor_set_allocation_infos),
            push_constant_ranges(push_constant_ranges)
        { }
    };

}