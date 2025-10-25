#include "MeshViewer.h"

#include <assimp/postprocess.h>
#include <chrono>
#include <numbers>
#include <Eigen/Dense>

#include "ember/graphics/Projections.h"
#include "ember/util/Log.h"

#define ASSET_PATH(x) (std::filesystem::path(MESHVIEWER_ASSET_PATH x))
#define RADIANS(x) ((x) * std::numbers::pi_v<float> / 180.0)

struct Vertex {
    Eigen::Vector3f pos;
    Eigen::Vector3f norm;
};

void MeshViewer::create_window() {
    m_vkinstance = VulkanInstance::create({
        .features {
            .vk_layer_validation = true,
            // .vk_layer_api_dump = true
        }
    });
    m_window = std::make_unique<Window>(m_vkinstance, "Mesh Viewer", 1920, 1080);
    m_graphics_device = GraphicsDevice::create(m_vkinstance, m_window->surface());
    m_renderer = m_window->create_renderer(m_graphics_device);
}

void MeshViewer::create_graphics_pipeline() {
    m_shaders[0] = m_renderer->create_shader_module(ASSET_PATH("shader.vert"));
    m_shaders[1] = m_renderer->create_shader_module(ASSET_PATH("shader.frag"));

    m_descriptor_set_blueprints = m_renderer->create_descriptor_set_blueprints(m_shaders);

    const std::array<Renderer::ShaderStageInfo, 2> shader_stages {
        Renderer::ShaderStageInfo{ .module = m_shaders[0] },
        Renderer::ShaderStageInfo{ .module = m_shaders[1] }
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

    const auto swapchain_format = m_window->get_swapchain_format();

    // const vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {
    //     .depthTestEnable = vk::True,
    //     .depthWriteEnable = vk::True,
    //     .depthCompareOp = vk::CompareOp::eLess,
    //     .stencilTestEnable = vk::False,
    // };

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
        // .depth_stencil_state = &depth_stencil_state,
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
    m_pipeline = m_renderer->create_graphics_pipeline(
        shader_stages,
        pipeline_state,
        m_descriptor_set_blueprints
    );
}

void MeshViewer::upload_model_data(const char* model_path) {
    info(MESHVIEWER_LOG, "Loading model {}", model_path);
    m_scene = m_assimp.ReadFile(
        model_path,
        aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals
    );

    info(MESHVIEWER_LOG, "Model has {} meshes", m_scene->mNumMeshes);
    info(MESHVIEWER_LOG, "Model has {} vertices", m_scene->mMeshes[0]->mNumVertices);
    info(MESHVIEWER_LOG, "Model has {} indices", m_scene->mMeshes[0]->mNumFaces * 3);

    std::vector<Vertex> vertices(m_scene->mMeshes[0]->mNumVertices);
    for (auto i = 0; i < vertices.size(); i++) {
        vertices[i].pos[0] = m_scene->mMeshes[0]->mVertices[i].x;
        vertices[i].pos[1] = m_scene->mMeshes[0]->mVertices[i].y;
        vertices[i].pos[2] = m_scene->mMeshes[0]->mVertices[i].z;

        vertices[i].norm[0] = m_scene->mMeshes[0]->mNormals[i].x;
        vertices[i].norm[1] = m_scene->mMeshes[0]->mNormals[i].y;
        vertices[i].norm[2] = m_scene->mMeshes[0]->mNormals[i].z;
    }

    m_vertex_buffer = m_renderer->create_vertex_buffer(
        sizeof(Vertex) * vertices.size(),
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    m_renderer->write_buffer<Vertex>(m_vertex_buffer, vertices);

    std::vector<uint32_t> indices;
    indices.reserve(m_scene->mMeshes[0]->mNumFaces * 3);

    for (auto i = 0; i < m_scene->mMeshes[0]->mNumFaces; i++) {
        indices.push_back(m_scene->mMeshes[0]->mFaces[i].mIndices[0]);
        indices.push_back(m_scene->mMeshes[0]->mFaces[i].mIndices[1]);
        indices.push_back(m_scene->mMeshes[0]->mFaces[i].mIndices[2]);
    }
    m_index_count = indices.size();

    m_index_buffer = m_renderer->create_index_buffer(
        sizeof(uint32_t) * indices.size(),
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    m_renderer->write_buffer<uint32_t>(m_index_buffer, indices);

    static_assert(sizeof(m_model_mvp) == 3 * sizeof(float) * 16);
    m_model_mvp.model = Eigen::Affine3f::Identity();
    m_model_mvp.view = m_camera.view_matrix();

    auto window_size = m_window->size();
    m_model_mvp.proj = perspective_projection(
        RADIANS(60.0),
        float(window_size.first)/window_size.second,
        0.1
    );

    m_uniform_buffer = m_renderer->create_buffer(
        sizeof(ModelViewProjection),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    m_renderer->write_buffer(m_uniform_buffer, m_model_mvp.model.data(), 0, sizeof(ModelViewProjection));
    m_ubo = m_renderer->create_descriptor_sets(m_descriptor_set_blueprints[0], 1).front();
    m_renderer->bind_buffers(
        m_descriptor_set_blueprints[0],
        std::array{m_ubo},
        std::array{m_uniform_buffer},
        0
    );
}

MeshViewer::MeshViewer(const char* model_path): m_camera({0.0, 0.0, -25.0}) {
    m_camera.set_min_distance(5.0);
    m_camera.set_max_distance(50.0);

    create_window();
    create_graphics_pipeline();
    upload_model_data(model_path);
}

MeshViewer::~MeshViewer() {
    m_renderer->wait_idle();
    m_renderer->destroy_descriptor_sets(m_descriptor_set_blueprints[0], std::array{m_ubo});
    m_renderer->destroy_buffer(m_uniform_buffer);
    m_renderer->destroy_buffer(m_vertex_buffer);
    m_renderer->destroy_buffer(m_index_buffer);
    m_renderer->destroy_pipeline(m_pipeline);
    m_renderer->destroy_descriptor_set_blueprints(m_descriptor_set_blueprints);
    for (auto& shader : m_shaders) {
        m_renderer->destroy_shader_module(shader);
    }
}

void MeshViewer::update_model_mvp() {
    static auto last_time = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
    last_time = currentTime;

    m_camera.add_rotation(Eigen::AngleAxisf(RADIANS(4.0 * time), Eigen::Vector3f::UnitX()));
    m_camera.add_rotation(Eigen::AngleAxisf(RADIANS(12.0 * time), Eigen::Vector3f::UnitY()));
    m_camera.add_rotation(Eigen::AngleAxisf(RADIANS(10.0 * time), Eigen::Vector3f::UnitZ()));

    m_camera.add_distance(25 * std::sin(RADIANS(-10.0 * time)));

    m_model_mvp.view = m_camera.view_matrix();
    m_renderer->write_buffer(m_uniform_buffer, m_model_mvp.model.data(), 0, sizeof(ModelViewProjection));
}

void MeshViewer::run() {
    m_window->set_visible(true);

    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }

        update_model_mvp();

        auto command_buffer = m_window->begin_rendering_frame();

        m_renderer->record_command_buffer(command_buffer, [this](CommandRecorder& recorder) {
            recorder.bind_pipeline(m_pipeline);
            recorder.bind_descriptor_sets(m_pipeline, 0, std::array{m_ubo});
            recorder.bind_vertex_buffer(m_vertex_buffer);
            recorder.bind_index_buffer(m_index_buffer, vk::IndexType::eUint32);
            recorder.draw_indexed(m_index_count, 1);
        });

        m_window->present_frame();
    }
}