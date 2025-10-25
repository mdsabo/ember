#include "MeshRenderSystem.h"

#include "Vertex.h"
#include "ember/ecs/TransformComponent.h"

namespace ember::graphics {

    using namespace gpu;

    void MeshRenderSystem::init(ecs::World& world) {
        world.add_component<MeshComponent>();
        world.add_component<SceneComponent>();
        world.add_component<CameraComponent>();
    }

    MeshRenderSystem::MeshRenderSystem(Window* window) {
        auto renderer = window->get_renderer();

        m_shaders[0] = renderer->create_shader_module(EMBER_ASSET_PATH "shaders/mesh_render_system.vert");
        m_shaders[1] = renderer->create_shader_module(EMBER_ASSET_PATH "shaders/mesh_render_system.frag");

        m_descriptor_set_blueprints = renderer->create_descriptor_set_blueprints(m_shaders);

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

        const auto swapchain_format = window->get_swapchain_format();

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
        m_pipeline = renderer->create_graphics_pipeline(
            shader_stages,
            pipeline_state,
            m_descriptor_set_blueprints
        );

        m_uniform_buffer = renderer->create_uniform_buffer(sizeof(Eigen::Matrix4f));
        m_uniform_staging_buffer = renderer->create_buffer(
            sizeof(Eigen::Matrix4f),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached
        );
        m_ubo = renderer->create_descriptor_sets(m_descriptor_set_blueprints[0], 1).front();
        renderer->bind_buffers(
            m_descriptor_set_blueprints[0],
            std::array{m_ubo},
            std::array{m_uniform_buffer},
            0
        );
    }

    namespace {
        Eigen::Matrix4f get_camera_vp_matrix(
            ecs::Entity ce,
            const CameraComponent& camera,
            const ecs::World& world
        ) {
            const auto& transforms = world.read_component<ecs::TransformComponent>();
            const auto camera_transform = transforms.at(ce).transform;
            const auto target_position = transforms
                .at(camera.focal_point)
                .transform
                .translation();

            const Eigen::Vector3f camera_pos = camera_transform.translation();
            const auto translation = Eigen::Translation3f(-Eigen::Vector3f::UnitZ() * camera_pos.norm());
            const auto rotation = Eigen::Quaternionf::FromTwoVectors(
                Eigen::Vector3f::UnitZ(),
                camera_pos - target_position
            ).toRotationMatrix();

            const auto view_matrix = Eigen::Affine3f((rotation * translation).inverse());
            return camera.projection * view_matrix.matrix();
        }
    }

    void MeshRenderSystem::pre_render(ecs::World& world, Window* window) {
        auto renderer = window->get_renderer();
        auto command_buffer = window->get_active_command_buffer();

        auto& camera_components = world.read_component<CameraComponent>();

        for (const auto& [ce, camera] : camera_components) {
            auto view_projection_matrix = get_camera_vp_matrix(ce, camera, world);
            renderer->write_buffer<Eigen::Matrix4f>(m_uniform_staging_buffer, std::array{view_projection_matrix});

            renderer->record_submit_command_buffer([this, &camera, &world](gpu::CommandRecorder& recorder) {
                recorder.copy_buffer(m_uniform_buffer, m_uniform_staging_buffer);
            });
        }
    }


    void MeshRenderSystem::render(ecs::World& world, Window* window) {
        auto renderer = window->get_renderer();
        auto command_buffer = window->get_active_command_buffer();

        auto& camera_components = world.read_component<CameraComponent>();

        renderer->record_command_buffer(command_buffer, [this](gpu::CommandRecorder& recorder) {
                recorder.bind_pipeline(m_pipeline);
                recorder.bind_descriptor_sets(m_pipeline, 0, std::array{m_ubo});
        });

        for (const auto& [ce, camera] : camera_components) {
            renderer->record_command_buffer(command_buffer, [this, &camera, &world](gpu::CommandRecorder& recorder) {
                // Setup camera details
                recorder.set_viewport(camera.viewport);

                auto& scenes = world.read_component<SceneComponent>();
                auto& meshes = world.read_component<MeshComponent>();

                for (const auto& mesh : meshes) {
                    const auto& mesh_data = scenes.at(mesh.scene).meshes.at(mesh.mesh_id);
                    recorder.bind_vertex_buffer(mesh_data.vertex_buffer);
                    recorder.bind_index_buffer(mesh_data.index_buffer);
                    recorder.draw_indexed(mesh_data.index_count, 1);
                }
            });
        }
    }

}