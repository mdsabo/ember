#pragma once

#include "RenderObjects.h"
#include "src/Vulkan.h"

namespace ember::gpu {

    class CommandRecorder {
    public:
        CommandRecorder(vk::CommandBuffer& command_buffer);

        using BufferCopy = vk::BufferCopy;
        void copy_buffer(const Buffer& dst, const Buffer& src, const std::vector<BufferCopy>& copies = {});

        void bind_pipeline(const Pipeline& pipeline);

        void bind_descriptor_sets(
            const Pipeline& pipeline,
            uint32_t first_set,
            const DescriptorSetChunk& descriptor_sets
        );

        void dispatch_compute(uint32_t x, uint32_t y, uint32_t z);

    private:
        vk::CommandBuffer& m_command_buffer;
    };

}