#pragma once

#include <vector>
#include "src/Vulkan.h"

namespace ember::gpu {

    class [[no_discard]] Buffer {
        friend class Renderer;
        friend class CommandRecorder;

        vk::DeviceSize size;
        vk::Buffer buffer;
        vk::DeviceMemory memory;

        Buffer() = default;
        Buffer(vk::DeviceSize size, vk::Buffer buffer, vk::DeviceMemory memory):
            size(size), buffer(buffer), memory(memory)
        { }
    };

    class [[no_discard]] DescriptorSetChunk {
        friend class Renderer;
        friend class CommandRecorder;

        std::vector<vk::DescriptorSet> sets;
        uint32_t set_index;

        DescriptorSetChunk() = default;
        DescriptorSetChunk(const std::vector<vk::DescriptorSet>& sets, uint32_t set_index):
            sets(sets), set_index(set_index)
        { }
    };

    class [[no_discard]] Pipeline {
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

    class [[no_discard]] ShaderModule {
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

        vk::DescriptorType get_descriptor_type(uint32_t set, uint32_t binding) const {
            return descriptor_set_layout_bindings.at(set).at(binding).descriptorType;
        }
    };

    class [[no_discard]] CommandBuffer {
        friend class Renderer;

        vk::CommandBuffer cmd_buffer;

        CommandBuffer() = default;
        CommandBuffer(vk::CommandBuffer cmd_buffer): cmd_buffer(cmd_buffer) { }
    };

    // Renderer::wait_for_fences RELIES ON THE FACT THAT THIS CLASS ONLY HAS A vk::Fence
    // IF THAT CHANGES wait_for_fences WILL BREAK
    class [[no_discard]] Fence {
        friend class Renderer;

        vk::Fence fence;

        Fence() = default;
        Fence(vk::Fence fence): fence(fence) { }
    };

    class [[no_discard]] Semaphore {
        friend class Renderer;

        vk::Semaphore semaphore;

        Semaphore() = default;
        Semaphore(vk::Semaphore semaphore): semaphore(semaphore) { }
    };

}