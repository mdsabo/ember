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

    constexpr auto NUM_ELEMENTS = 64;
    constexpr auto BUFFER_SIZE = sizeof(uint32_t)*NUM_ELEMENTS;
    auto src_host_buffer = renderer.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto src_gpu_buffer = renderer.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    auto dst_host_buffer = renderer.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto dst_gpu_buffer = renderer.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    std::vector<uint32_t> data(NUM_ELEMENTS, 0.0);
    renderer.write_buffer(dst_host_buffer, data.data(), 0, BUFFER_SIZE);
    for (uint32_t i = 0; i < NUM_ELEMENTS; i++) data[i] = i;
    renderer.write_buffer(src_host_buffer, data.data(), 0, BUFFER_SIZE);

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR).append("tests/compute_squares.comp");
    auto shader_module = renderer.create_shader_module(shader_path);
    auto pipeline = renderer.create_compute_pipeline(shader_module);

    auto descriptor_sets = renderer.create_descriptor_sets(shader_module, 0, 1);
    renderer.bind_buffers(shader_module, descriptor_sets, {}, { src_gpu_buffer, dst_gpu_buffer });

    auto command_buffer = renderer.record_command_buffer(true, [&](CommandRecorder& recorder) {
        recorder.copy_buffer(src_gpu_buffer, src_host_buffer);
        recorder.bind_pipeline(pipeline);
        recorder.bind_descriptor_sets(pipeline, 0, descriptor_sets);
        recorder.dispatch_compute(NUM_ELEMENTS/16, 1, 1);
        recorder.copy_buffer(dst_host_buffer, dst_gpu_buffer);
    });

    auto fence = renderer.submit_command_buffer(command_buffer);

    renderer.wait_for_fences({ fence });

    std::vector<uint32_t> squared(NUM_ELEMENTS);
    renderer.read_buffer(squared.data(), dst_host_buffer, 0, BUFFER_SIZE);

    renderer.destroy_command_buffer(command_buffer);
    renderer.destroy_descriptor_sets(shader_module, descriptor_sets);
    renderer.destroy_pipeline(pipeline);
    renderer.destroy_shader_module(shader_module);
    renderer.destroy_buffer(src_host_buffer);
    renderer.destroy_buffer(src_gpu_buffer);
    renderer.destroy_buffer(dst_host_buffer);
    renderer.destroy_buffer(dst_gpu_buffer);

    for (uint32_t i = 0; i < NUM_ELEMENTS; i+=4) {
        if (squared[i] != (i*i)) {
            printf("Assertion failed %u != %u\n", squared[i], i*i);
            return -1;
        }
    }
    return 0;
}