#pragma once

#include "DescriptorSetChunk.h"
#include "Pipeline.h"
#include "Vulkan.h"

namespace ember::gpu {

    class CommandRecorder {
    public:
        CommandRecorder(vk::CommandBuffer& command_buffer);

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