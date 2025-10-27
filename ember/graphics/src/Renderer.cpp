#include "Renderer.h"

#include "CameraComponent.h"

#include "ember/gpu/RenderObjects.h"
#include "ember/gpu/VulkanHelpers.h"

namespace ember::graphics {

    void Renderer::init(ecs::World& world) {
        world.add_component<CameraComponent>();
    }

    Renderer::Renderer(
        std::shared_ptr<const gpu::GPUDevice> gpu_device,
        std::unique_ptr<Window> window
    ): m_gpu_device(gpu_device), m_window(std::move(window)) {
        m_gpu_interface = std::make_unique<gpu::GPUInterface>(m_gpu_device);

        m_renderpasses.push_back(
            std::make_unique<SwapchainRenderpass>(m_gpu_interface.get(), m_window.get())
        );
    }

    Renderer::~Renderer() {
        m_gpu_interface->wait_idle();

        for (auto& renderpass : m_renderpasses) {
            renderpass->destroy(m_gpu_interface.get());
        }

        m_gpu_interface = nullptr;
        m_window = nullptr;
    }

    void Renderer::render(ecs::World& world) {
        auto gpu_interface = m_gpu_interface.get();
        auto& cameras = world.read_component<CameraComponent>();
        for (auto [e, camera] : cameras) {
            for (auto& renderpass : m_renderpasses) {
                m_renderpass_objects = renderpass->begin(gpu_interface);

                // Run every system?

                renderpass->end(gpu_interface);
            }
        }
    }

    void Renderer::draw_mesh(uint64_t mesh_id, const glm::mat4& transform) {
        m_gpu_interface->record_command_buffer(m_renderpass_objects.command_buffer, [&](gpu::CommandRecorder& recorder) {
            recorder.push_constants(
                m_renderpass_objects.pipeline,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(glm::mat4),
                &transform
            );
            // recorder.bind_vertex_buffer(mesh_data.vertex_buffer);
            // recorder.bind_index_buffer(mesh_data.index_buffer);
            // recorder.draw_indexed(mesh_data.index_count, 1);
        });
    }

}



// glm::mat4 get_camera_vp_matrix(
//     ecs::Entity ce,
//     const CameraComponent& camera,
//     const ecs::World& world
// ) {
//     const auto& transforms = world.read_component<ecs::TransformComponent>();
//     const auto eye = glm::vec3(transforms.at(ce).transform[3]);
//     const auto center = glm::vec3(transforms.at(camera.focal_point).transform[3]);
//     auto view_matrix = glm::lookAt(eye, center, CameraComponent::UP_VECTOR);
//     auto proj = camera.projection;
//     proj[1][1] *= -1;
//     return proj * view_matrix;
// }