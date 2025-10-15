#include "Renderer.h"

#include "Util.h"

namespace ember::gpu {

    Renderer::Renderer(std::shared_ptr<const GraphicsDevice> device): m_gpu(device) {
        auto [vk_device, queue] = device->create_device_and_queue();
        m_device = vk_device;
        m_queue = queue;
    }

    Renderer::~Renderer() {
        m_device.destroy();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // MEMORY                                                                                   //
    //////////////////////////////////////////////////////////////////////////////////////////////

    namespace {
        uint32_t find_memory_type_index(
            vk::PhysicalDeviceMemoryProperties device_memory_properties,
            vk::MemoryPropertyFlags flags,
            uint32_t type_filter
        ) {
            for (auto i = 0; i < device_memory_properties.memoryTypeCount; i++) {
                auto has_type = bool((1ULL << i) & type_filter);
                auto has_properties = vkFlagContains(device_memory_properties.memoryTypes[i].propertyFlags, flags);
                if (has_type && has_properties) return i;
            }
            throw std::runtime_error("No suitable memory types found!");
        }
    } // namespace

    vk::DeviceMemory Renderer::allocate_memory(vk::DeviceSize size, vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties) {
        auto memory_type_index = find_memory_type_index(
            m_gpu->memory_properties(), 
            properties, 
            requirements.memoryTypeBits
        );

        const vk::MemoryAllocateInfo alloc_info{
            .allocationSize = size,
            .memoryTypeIndex = memory_type_index
        };
        return m_device.allocateMemory(alloc_info);
    }

    Buffer Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        const vk::BufferCreateInfo buffer_create_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };
        
        auto buffer = m_device.createBuffer(buffer_create_info);
        auto memory_requirements = m_device.getBufferMemoryRequirements(buffer);
        auto memory = allocate_memory(size, memory_requirements, properties);
        m_device.bindBufferMemory(buffer, memory, 0);
        
        return Buffer{ size, buffer, memory};
    }

    void Renderer::destroy_buffer(Buffer& buffer) {
        m_device.freeMemory(buffer.memory);
        m_device.destroyBuffer(buffer.buffer);
    }
}