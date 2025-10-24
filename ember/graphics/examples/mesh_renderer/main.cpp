#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "ember/gpu/Renderer.h"
#include "ember/gpu/VulkanHelpers.h"
#include "ember/util/Log.h"
#include "Window.h"

using namespace ember::gpu;
using namespace ember::graphics;

#define RESOURCE_PATH(x) (std::filesystem::path(EMBER_GRAPHICS_DIR "examples/mesh_renderer/" x))

struct Vertex {
    float pos[3];
    float norm[3];
};


class MeshRendererApp {
public:
    MeshRendererApp() {
        const auto cube_path = RESOURCE_PATH("cube.fbx");
        cube_model = assimp_importer.ReadFile(
            cube_path.string(),
            aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
        );

        vkinstance = VulkanInstance::create({
            .features {
                .vk_layer_validation = true,
                // .vk_layer_api_dump = true
            }
        });
        window = Window(vkinstance, "Mesh Renderer", 800, 600);
        graphics_device = GraphicsDevice::create(vkinstance, window.surface());
        renderer = window.create_renderer(graphics_device);

        auto vertex_shader_path = std::filesystem::path(EMBER_GRAPHICS_DIR "examples/mesh_renderer/shader.vert");
        auto fragment_shader_path = std::filesystem::path(EMBER_GRAPHICS_DIR "examples/mesh_renderer/shader.frag");

        shaders[0] = renderer->create_shader_module(vertex_shader_path);
        shaders[1] = renderer->create_shader_module(fragment_shader_path);

        descriptor_set_blueprints = renderer->create_descriptor_set_blueprints(shaders);

        const std::array<Renderer::ShaderStageInfo, 2> shader_stages {
            Renderer::ShaderStageInfo{ .module = shaders[0] },
            Renderer::ShaderStageInfo{ .module = shaders[1] }
        };
        const std::array vertex_bindings{
            vk::VertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = vk::VertexInputRate::eVertex
            }
        };
        const vk::PipelineColorBlendAttachmentState color_blend_attachment {
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA,
        };

        const auto swapchain_format = window.get_swapchain_format();

        // see https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        const Renderer::GraphicsPipelineState pipeline_state {
            .vertex_bindings = vertex_bindings,
            .input_assembly_state = vk::PipelineInputAssemblyStateCreateInfo {
                .topology = vk::PrimitiveTopology::eTriangleList,
            },
            .rasterization_state = vk::PipelineRasterizationStateCreateInfo {
                .depthClampEnable = vk::False,
                .rasterizerDiscardEnable = vk::False,
                .polygonMode = vk::PolygonMode::eFill,
                .cullMode = vk::CullModeFlagBits::eBack,
                .frontFace = vk::FrontFace::eCounterClockwise,
                .depthBiasEnable = vk::False,
                .lineWidth = 1.0,
            },
            .multisample_state = vk::PipelineMultisampleStateCreateInfo {
                .rasterizationSamples = vk::SampleCountFlagBits::e1,
                .sampleShadingEnable = vk::False,
            },
            .color_blend_state = vk::PipelineColorBlendStateCreateInfo {
                .logicOpEnable = vk::False,
                .attachmentCount = 1,
                .pAttachments = &color_blend_attachment
            },
            .rendering_info = vk::PipelineRenderingCreateInfo {
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &swapchain_format,
            }
        };
        pipeline = renderer->create_graphics_pipeline(
            shader_stages,
            pipeline_state,
            descriptor_set_blueprints
        );

        upload_model_data();
    }

    void upload_model_data() {
        info(EMBER_GRAPHICS_LOG, "Cube has {} meshes", cube_model->mNumMeshes);
        info(EMBER_GRAPHICS_LOG, "Cube has {} vertices", cube_model->mMeshes[0]->mNumVertices);
        info(EMBER_GRAPHICS_LOG, "Cube has {} indices", cube_model->mMeshes[0]->mNumFaces * 3);

        std::vector<Vertex> vertices(cube_model->mMeshes[0]->mNumVertices);
        for (auto i = 0; i < vertices.size(); i++) {
            vertices[i].pos[0] = cube_model->mMeshes[0]->mVertices[i].x / 2.0;
            vertices[i].pos[1] = cube_model->mMeshes[0]->mVertices[i].y / 2.0;
            vertices[i].pos[2] = cube_model->mMeshes[0]->mVertices[i].z / 2.0;

            vertices[i].norm[0] = cube_model->mMeshes[0]->mNormals[i].x;
            vertices[i].norm[1] = cube_model->mMeshes[0]->mNormals[i].y;
            vertices[i].norm[2] = cube_model->mMeshes[0]->mNormals[i].z;
        }

        vertex_buffer = renderer->create_vertex_buffer(
            sizeof(Vertex) * vertices.size(),
            vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        renderer->write_buffer<Vertex>(vertex_buffer, vertices);

        std::vector<uint32_t> indices;
        indices.reserve(cube_model->mMeshes[0]->mNumFaces * 3);

        for (auto i = 0; i < cube_model->mMeshes[0]->mNumFaces; i++) {
            indices.push_back(cube_model->mMeshes[0]->mFaces[i].mIndices[0]);
            indices.push_back(cube_model->mMeshes[0]->mFaces[i].mIndices[1]);
            indices.push_back(cube_model->mMeshes[0]->mFaces[i].mIndices[2]);
        }
        index_count = indices.size();

        index_buffer = renderer->create_index_buffer(
            sizeof(uint32_t) * indices.size(),
            vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        renderer->write_buffer<uint32_t>(index_buffer, indices);
    }

    ~MeshRendererApp() {
        renderer->wait_idle();
        renderer->destroy_buffer(vertex_buffer);
        renderer->destroy_buffer(index_buffer);
        renderer->destroy_pipeline(pipeline);
        renderer->destroy_descriptor_set_blueprints(descriptor_set_blueprints);
        for (auto& shader : shaders) {
            renderer->destroy_shader_module(shader);
        }
    }

    void run() {
        window.set_visible(true);

        SDL_Event e;
        SDL_zero(e);
        bool quit = false;
        while (quit == false) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) quit = true;
            }

            auto command_buffer = window.begin_rendering_frame();

            renderer->record_command_buffer(command_buffer, [&](CommandRecorder& recorder) {
                recorder.bind_pipeline(pipeline);
                recorder.bind_vertex_buffer(vertex_buffer);
                recorder.bind_index_buffer(index_buffer, vk::IndexType::eUint32);
                recorder.draw_indexed(index_count, 1);
            });

            window.present_frame();
        }
    }

private:
    std::shared_ptr<const VulkanInstance> vkinstance;
    std::shared_ptr<const GraphicsDevice> graphics_device;
    Window window;
    Renderer* renderer;

    std::array<ShaderModule*, 2> shaders;
    DescriptorSetArray<DescriptorSetBlueprint*> descriptor_set_blueprints;
    Pipeline* pipeline;
    Buffer* vertex_buffer, *index_buffer, *staging_buffer;
    uint32_t index_count;

    Assimp::Importer assimp_importer;
    const aiScene* cube_model;
};


int main(int argc, char* args[]) {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());
    MeshRendererApp().run();
    return 0;
}