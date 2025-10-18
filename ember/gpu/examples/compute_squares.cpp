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

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR).append("examples/compute_squares.comp");
    auto shader_module = renderer.create_shader_module(shader_path);
    auto pipeline = renderer.create_compute_pipeline(shader_module);

    auto descriptor_sets = renderer.create_descriptor_sets(shader_module, 0, 1);

    auto command_buffer = renderer.record_render_commands([&](CommandRecorder& recorder) {
        recorder.bind_pipeline(pipeline);
        recorder.bind_descriptor_sets(pipeline, 0, descriptor_sets);
        recorder.dispatch_compute(1, 1, 1);
    });

    renderer.submit_command_buffer(command_buffer);

    renderer.destroy_descriptor_sets(shader_module, descriptor_sets);
    renderer.destroy_pipeline(pipeline);
    renderer.destroy_shader_module(shader_module);
    renderer.destroy_buffer(src_buffer);
    renderer.destroy_buffer(dst_buffer);
    return 0;
}