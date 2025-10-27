
#include <glm/glm.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <array>
#include <fstream>
#include <vector>

#include "GPUInterface.h"
#include "VulkanHelpers.h"
#include "ember/util/ArgParser.h"
#include "ember/util/Log.h"

using namespace ember::gpu;
using namespace ember::util;

using Vertex = glm::vec2;

static std::vector<Vertex> get_vertices() {
    return {
        { -0.5, -0.5 },
        { 0, 0.5 },
        { -0.5, 0.5 },
    };
}

int main(int argc, const char* argv[]) {
    // set_global_logger(std::make_unique<PrintfLogger>());

    auto argparse = ArgParser(argc, argv);
    //argparse.add_argument(nullptr, "--api-dump", "Enable Vulkan API dump layer");

    const AppInfo app_info {
        .name = "compute_raytracer",
        .version = 1,
        .features = EngineFeatures {
            .vk_layer_validation = true,
            .vk_layer_api_dump = argparse.is_set("--api-dump"),
        }
    };

    auto instance = VulkanInstance::create(app_info);
    auto device = GPUDevice::create(instance);
    auto gfxinterface = GPUInterface(device);

    auto vertices = get_vertices();
    const auto vertex_buffer_size = sizeof(Vertex)*vertices.size();

    auto vertex_staging_buffer = gfxinterface.create_buffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto vertex_buffer = gfxinterface.create_buffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    gfxinterface.write_buffer(vertex_buffer, vertices.begin(), vertices.end());

    constexpr auto IMAGE_DIM_X = 1024;
    constexpr auto IMAGE_DIM_Y = 768;
    auto output_image = gfxinterface.create_image(GPUInterface::ImageCreateInfo {
        .type = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Uint,
        .extent = vk::Extent3D{ .width = IMAGE_DIM_X, .height = IMAGE_DIM_Y, .depth = 1 },
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
        .layout = vk::ImageLayout::eGeneral,
        .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    });
    auto output_staging_buffer = gfxinterface.create_buffer(
        IMAGE_DIM_X*IMAGE_DIM_Y*4,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/raytracer.comp");
    auto shader_module = gfxinterface.create_shader_module(shader_path);

    auto descriptor_set_blueprints = gfxinterface.create_descriptor_set_blueprints(std::array{shader_module});
    auto descriptor_sets = gfxinterface.create_descriptor_sets(descriptor_set_blueprints[0], 1);
    gfxinterface.bind_buffers(descriptor_set_blueprints[0], descriptor_sets, std::array{ vertex_buffer }, 0);
    gfxinterface.bind_images(descriptor_set_blueprints[0], descriptor_sets, std::array{ output_image }, 1);

    auto pipeline = gfxinterface.create_compute_pipeline({ shader_module }, descriptor_set_blueprints);

    auto command_buffer = gfxinterface.create_command_buffer();
    gfxinterface.record_submit_command_buffer([&](CommandRecorder& recorder) {
        recorder.copy_buffer(vertex_buffer, vertex_staging_buffer);
        recorder.bind_pipeline(pipeline);
        recorder.bind_descriptor_sets(pipeline, 0, descriptor_sets);
        recorder.dispatch_compute(IMAGE_DIM_X/32, IMAGE_DIM_Y/32, 1);

        const CommandRecorder::ImageTransitionInfo info {
            .new_layout = vk::ImageLayout::eTransferSrcOptimal,
            .src_pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .dst_pipeline_stage = vk::PipelineStageFlagBits::eTransfer,
            .src_access_mask = vk::AccessFlagBits::eShaderWrite,
            .dst_access_mask = vk::AccessFlagBits::eTransferRead,
            .subresource_range = MAX_SUBRESOURCE_RANGE(vk::ImageAspectFlagBits::eColor)
        };
        recorder.transition_image_layout(output_image, info);
        recorder.copy_image_to_buffer(output_staging_buffer, output_image);
    });

    std::vector<uint32_t> image_data(IMAGE_DIM_X*IMAGE_DIM_Y);
    gfxinterface.read_buffer(image_data.data(), output_staging_buffer, 0, IMAGE_DIM_X*IMAGE_DIM_Y*4);

    gfxinterface.destroy_pipeline(pipeline);
    gfxinterface.destroy_descriptor_sets(descriptor_set_blueprints[0], descriptor_sets);
    gfxinterface.destroy_descriptor_set_blueprints(descriptor_set_blueprints);
    gfxinterface.destroy_shader_module(shader_module);
    gfxinterface.destroy_buffer(output_staging_buffer);
    gfxinterface.destroy_image(output_image);
    gfxinterface.destroy_buffer(vertex_staging_buffer);
    gfxinterface.destroy_buffer(vertex_buffer);

    auto output_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/raytracer.output.bmp")
        .string();
    stbi_write_bmp(output_path.c_str(), IMAGE_DIM_X, IMAGE_DIM_Y, 4, image_data.data());
}