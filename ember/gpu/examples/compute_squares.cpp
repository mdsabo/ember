#include "Renderer.h"

using namespace ember::gpu;

int main() {
    const auto app_info = AppInfo {
        .name = "compute_squares",
        .version = 1,
        .features = EngineFeatures {
            .vk_layer_validation = true,
            .vk_layer_api_dump = true,
        }
    };

    auto device = GraphicsDevice::create(app_info);
    auto renderer = Renderer(device);

    auto src_buffer = renderer.create_buffer(
        sizeof(float)*64,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    auto dst_buffer = renderer.create_buffer(
        sizeof(float)*64,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );



    renderer.destroy_buffer(src_buffer);
    renderer.destroy_buffer(dst_buffer);
    return 0;
}