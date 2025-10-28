#include "GPUInterface.h"
#include "ember/util/ArgParser.h"
#include "ember/util/Log.h"

using namespace ember::gpu;
using namespace ember::util;

int main(int argc, const char* argv[]) {
    set_global_logger(std::make_unique<PrintfLogger>());

    auto argparse = ArgParser(argc, argv);

    const AppInfo app_info {
        .name = "compute_squares",
        .version = 1,
        .features = EngineFeatures {
            .vk_layer_validation = true,
            .vk_layer_api_dump = argparse.is_set("--api-dump"),
        }
    };

    auto instance = VulkanInstance::create(app_info);
    auto device = GPUDevice::create(instance);
    auto gfxinterface = GPUInterface(device);

    constexpr auto NUM_ELEMENTS = 64;
    constexpr auto BUFFER_SIZE = sizeof(uint32_t)*NUM_ELEMENTS;
    auto src_host_buffer = gfxinterface.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto src_gpu_buffer = gfxinterface.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    auto dst_host_buffer = gfxinterface.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto dst_gpu_buffer = gfxinterface.create_buffer(
        BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    std::vector<uint32_t> data(NUM_ELEMENTS, 0.0);
    gfxinterface.write_buffer(dst_host_buffer, data.data(), 0, BUFFER_SIZE);
    for (uint32_t i = 0; i < NUM_ELEMENTS; i++) data[i] = i;
    gfxinterface.write_buffer(src_host_buffer, data.data(), 0, BUFFER_SIZE);

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("tests/compute_squares.comp");
    auto shader_module = gfxinterface.create_shader_module(shader_path);

    auto descriptor_set_blueprints = gfxinterface.create_descriptor_set_blueprints(std::array{shader_module});
    auto descriptor_sets = gfxinterface.create_descriptor_sets(descriptor_set_blueprints[0], 1);
    gfxinterface.bind_buffers(
        descriptor_set_blueprints[0],
        descriptor_sets,
        std::array{ src_gpu_buffer, dst_gpu_buffer },
        0
    );

    auto pipeline = gfxinterface.create_compute_pipeline({ shader_module }, descriptor_set_blueprints);

    gfxinterface.record_submit_command_buffer([&](CommandRecorder& recorder) {
        recorder.copy_buffer(src_gpu_buffer, src_host_buffer);
        recorder.bind_pipeline(pipeline);
        recorder.bind_descriptor_sets(pipeline, 0, descriptor_sets);
        recorder.dispatch_compute(NUM_ELEMENTS/16, 1, 1);
        recorder.copy_buffer(dst_host_buffer, dst_gpu_buffer);
    });

    gfxinterface.wait_idle();

    std::vector<uint32_t> squared(NUM_ELEMENTS);
    gfxinterface.read_buffer(squared.data(), dst_host_buffer, 0, BUFFER_SIZE);

    gfxinterface.destroy_pipeline(pipeline);
    gfxinterface.destroy_descriptor_sets(descriptor_set_blueprints[0], descriptor_sets);
    gfxinterface.destroy_descriptor_set_blueprints(descriptor_set_blueprints);
    gfxinterface.destroy_shader_module(shader_module);
    gfxinterface.destroy_buffer(src_host_buffer);
    gfxinterface.destroy_buffer(src_gpu_buffer);
    gfxinterface.destroy_buffer(dst_host_buffer);
    gfxinterface.destroy_buffer(dst_gpu_buffer);

    for (uint32_t i = 0; i < NUM_ELEMENTS; i+=4) {
        if (squared[i] != (i*i)) {
            printf("Assertion failed %u != %u\n", squared[i], i*i);
            return -1;
        }
    }
    return 0;
}