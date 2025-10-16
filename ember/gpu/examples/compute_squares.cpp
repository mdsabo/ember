#include "Renderer.h"

#include "ember/util/ArgParser.h"

using namespace ember::gpu;
using namespace ember::util;

int main(int argc, const char* argv[]) {
    auto argparse = ArgParser(argc, argv);

    const AppInfo app_info {
        .name = "compute_squares",
        .version = 1,
        .features = EngineFeatures {
            .vk_layer_validation = true,
            .vk_layer_api_dump = argparse.is_set("--api-dump"),
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

    auto shader_module = renderer.create_shader_module("C:\\Users\\slabo\\Code\\ember\\ember\\gpu\\examples\\compute_squares.comp");
    auto pipeline = renderer.create_compute_pipeline(shader_module);

    

    renderer.destroy_pipeline(pipeline);
    renderer.destroy_shader_module(shader_module);
    renderer.destroy_buffer(src_buffer);
    renderer.destroy_buffer(dst_buffer);
    return 0;
}