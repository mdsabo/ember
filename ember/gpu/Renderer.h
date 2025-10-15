#pragma once

#include "Buffer.h"
#include "GraphicsDevice.h"

namespace ember::gpu {

    class Renderer {
    public:
        Renderer(std::shared_ptr<const GraphicsDevice> device);
        ~Renderer();

        Buffer create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void destroy_buffer(Buffer& buffer);

    private:
        std::shared_ptr<const GraphicsDevice> m_gpu;
        vk::Device m_device;
        vk::Queue m_queue;

        vk::DeviceMemory allocate_memory(
            vk::DeviceSize size,
            vk::MemoryRequirements requirements, 
            vk::MemoryPropertyFlags properties
        );
    };

}