
#include <Eigen/Dense>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <array>
#include <fstream>
#include <vector>

#include "Renderer.h"
#include "ember/util/ArgParser.h"
#include "ember/util/Log.h"

using namespace ember::gpu;
using namespace ember::util;

using Vertex = Eigen::Vector2f;

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

    auto device = GraphicsDevice::create(app_info);
    auto renderer = Renderer(device);

    auto vertices = get_vertices();
    const auto vertex_buffer_size = sizeof(Vertex)*vertices.size();

    auto vertex_staging_buffer = renderer.create_buffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    auto vertex_buffer = renderer.create_buffer(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    );

    renderer.write_buffer(vertex_buffer, vertices.begin(), vertices.end());

    constexpr auto IMAGE_DIM_X = 1024;
    constexpr auto IMAGE_DIM_Y = 768;
    auto output_image = renderer.create_image(Renderer::ImageCreateInfo {
        .type = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Uint,
        .extent = vk::Extent3D{ .width = IMAGE_DIM_X, .height = IMAGE_DIM_Y, .depth = 1 },
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
        .layout = vk::ImageLayout::eGeneral,
        .memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    });
    auto output_staging_buffer = renderer.create_buffer(
        IMAGE_DIM_X*IMAGE_DIM_Y*4,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    auto shader_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/raytracer.comp");
    auto shader_module = renderer.create_shader_module(shader_path);

    auto pipeline = renderer.create_compute_pipeline(shader_module);

    auto descriptor_sets = renderer.create_descriptor_sets(shader_module, 0, 1);
    renderer.bind_buffers(shader_module, descriptor_sets, {}, { vertex_buffer });
    renderer.bind_images(shader_module, descriptor_sets, { .binding_index = 1 }, { output_image });

    auto command_buffer = renderer.create_command_buffer();
    renderer.record_submit_command_buffer([&](CommandRecorder& recorder) {
        recorder.copy_buffer(vertex_buffer, vertex_staging_buffer);
        recorder.bind_pipeline(pipeline);
        recorder.bind_descriptor_sets(pipeline, 0, descriptor_sets);
        recorder.dispatch_compute(IMAGE_DIM_X/32, IMAGE_DIM_Y/32, 1);
        recorder.transition_image_layout(output_image, vk::ImageLayout::eTransferSrcOptimal);
        recorder.copy_image_to_buffer(output_staging_buffer, output_image);
    });

    std::vector<uint32_t> image_data(IMAGE_DIM_X*IMAGE_DIM_Y);
    renderer.read_buffer(image_data.data(), output_staging_buffer, 0, IMAGE_DIM_X*IMAGE_DIM_Y*4);

    renderer.destroy_descriptor_sets(shader_module, std::move(descriptor_sets));
    renderer.destroy_pipeline(std::move(pipeline));
    renderer.destroy_shader_module(std::move(shader_module));
    renderer.destroy_buffer(std::move(output_staging_buffer));
    renderer.destroy_image(std::move(output_image));
    renderer.destroy_buffer(std::move(vertex_staging_buffer));
    renderer.destroy_buffer(std::move(vertex_buffer));

    auto output_path = std::filesystem::path(EMBER_GPU_DIR)
        .append("examples/raytracer.output.bmp")
        .string();
    stbi_write_bmp(output_path.c_str(), IMAGE_DIM_X, IMAGE_DIM_Y, 4, image_data.data());
}