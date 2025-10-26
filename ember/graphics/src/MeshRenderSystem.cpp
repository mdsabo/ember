#include "MeshRenderSystem.h"

#include <glm/ext/matrix_transform.hpp>

#include "Vertex.h"
#include "ember/ecs/TransformComponent.h"

namespace ember::graphics {

    using namespace gpu;

    void MeshRenderSystem::init(ecs::World& world) {
        world.add_component<MeshComponent>();
        world.add_component<SceneComponent>();
        world.add_component<CameraComponent>();
    }

    MeshRenderSystem::MeshRenderSystem(Renderer* window) {
        auto gfxinterface = window->get_renderer();

        m_shaders[0] = gfxinterface->create_shader_module(EMBER_ASSET_PATH "shaders/mesh_render_system.vert");
        m_shaders[1] = gfxinterface->create_shader_module(EMBER_ASSET_PATH "shaders/mesh_render_system.frag");

        m_descriptor_set_blueprints = gfxinterface->create_descriptor_set_blueprints(m_shaders);

        const std::array<GraphicsInterface::ShaderStageInfo, 2> shader_stages {
            GraphicsInterface::ShaderStageInfo{ .module = m_shaders[0] },
            GraphicsInterface::ShaderStageInfo{ .module = m_shaders[1] }
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

        const auto swapchain_format = window->get_swapchain_format();

        const vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLessOrEqual,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False
        };

        // see https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        const GraphicsInterface::GraphicsPipelineState pipeline_state {
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
            .depth_stencil_state = &depth_stencil_state,
            .color_blend_state = vk::PipelineColorBlendStateCreateInfo {
                .logicOpEnable = vk::False,
                .attachmentCount = 1,
                .pAttachments = &color_blend_attachment
            },
            .rendering_info = vk::PipelineRenderingCreateInfo {
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &swapchain_format,
                .depthAttachmentFormat = vk::Format::eD32Sfloat,
            }
        };
        m_pipeline = gfxinterface->create_graphics_pipeline(
            shader_stages,
            pipeline_state,
            m_descriptor_set_blueprints
        );

        m_uniform_buffer = gfxinterface->create_uniform_buffer(sizeof(glm::mat4));
        m_uniform_staging_buffer = gfxinterface->create_buffer(
            sizeof(glm::mat4),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached
        );
        m_ubo = gfxinterface->create_descriptor_sets(m_descriptor_set_blueprints[0], 1).front();
        gfxinterface->bind_buffers(
            m_descriptor_set_blueprints[0],
            std::array{m_ubo},
            std::array{m_uniform_buffer},
            0
        );
    }

    namespace {
        glm::mat4 get_camera_vp_matrix(
            ecs::Entity ce,
            const CameraComponent& camera,
            const ecs::World& world
        ) {
            const auto& transforms = world.read_component<ecs::TransformComponent>();
            const auto eye = glm::vec3(transforms.at(ce).transform[3]);
            const auto center = glm::vec3(transforms.at(camera.focal_point).transform[3]);
            auto view_matrix = glm::lookAt(eye, center, CameraComponent::UP_VECTOR);
            auto proj = camera.projection;
            proj[1][1] *= -1;
            return proj * view_matrix;
        }
    }

    void MeshRenderSystem::pre_render(ecs::World& world, Renderer* window) {
        auto gfxinterface = window->get_renderer();

        auto& camera_components = world.read_component<CameraComponent>();

        for (const auto& [ce, camera] : camera_components) {
            auto view_projection_matrix = get_camera_vp_matrix(ce, camera, world);
            gfxinterface->write_buffer<glm::mat4>(m_uniform_staging_buffer, std::array{view_projection_matrix});

            gfxinterface->record_submit_command_buffer([this, &camera, &world](gpu::CommandRecorder& recorder) {
                recorder.copy_buffer(m_uniform_buffer, m_uniform_staging_buffer);
            });
        }
    }


    void MeshRenderSystem::render(ecs::World& world, Renderer* window) {
        auto gfxinterface = window->get_renderer();
        auto command_buffer = window->get_active_command_buffer();

        auto& camera_components = world.read_component<CameraComponent>();

        gfxinterface->record_command_buffer(command_buffer, [this](gpu::CommandRecorder& recorder) {
                recorder.bind_pipeline(m_pipeline);
                recorder.bind_descriptor_sets(m_pipeline, 0, std::array{m_ubo});
        });


        auto mesh_entities = world.query<ecs::TransformComponent, MeshComponent>().as_set();

        for (const auto& [ce, camera] : camera_components) {
            gfxinterface->record_command_buffer(command_buffer, [&](gpu::CommandRecorder& recorder) {
                // Setup camera details
                recorder.set_viewport(camera.viewport);

                auto& scenes = world.read_component<SceneComponent>();
                auto& meshes = world.read_component<MeshComponent>();
                auto& transforms = world.read_component<ecs::TransformComponent>();

                for (const auto e : mesh_entities) {
                    const auto& mesh = meshes.at(e);
                    const auto& transform = transforms.at(e);
                    const auto& mesh_data = scenes.at(mesh.scene).meshes.at(mesh.mesh_id);
                    recorder.push_constants(
                        m_pipeline,
                        vk::ShaderStageFlagBits::eVertex,
                        0,
                        sizeof(glm::mat4),
                        &transform.transform
                    );
                    recorder.bind_vertex_buffer(mesh_data.vertex_buffer);
                    recorder.bind_index_buffer(mesh_data.index_buffer);
                    recorder.draw_indexed(mesh_data.index_count, 1);
                }
            });
        }
    }

}