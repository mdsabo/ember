#include "CommandRecorder.h"

namespace ember::gpu {

    CommandRecorder::CommandRecorder(vk::CommandBuffer& command_buffer): m_command_buffer(command_buffer) { }

    void CommandRecorder::copy_buffer(const Buffer& dst, const Buffer& src, const std::vector<BufferCopy>& copies) {
        if (copies.size() == 0) {
            m_command_buffer.copyBuffer(
                src.buffer, 
                dst.buffer, 
                vk::BufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = std::min(src.size, dst.size)}
            );
        } else {
            m_command_buffer.copyBuffer(src.buffer, dst.buffer, copies);
        }
    }

    void CommandRecorder::bind_pipeline(const Pipeline& pipeline) {
        m_command_buffer.bindPipeline(pipeline.bind_point, pipeline.pipeline);
    }

    void CommandRecorder::bind_descriptor_sets(
        const Pipeline& pipeline, 
        uint32_t first_set,
        const DescriptorSetChunk& descriptor_sets
    ) {
        m_command_buffer.bindDescriptorSets(
            pipeline.bind_point, 
            pipeline.layout, 
            first_set, 
            descriptor_sets.sets, 
            {}
        );
    }

    void CommandRecorder::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
        m_command_buffer.dispatch(x, y, z);
    }

}