#pragma once

#include <array>

#include "Mesh.h"
#include "Renderpass.h"
#include "Window.h"

#include "ember/ecs/System.h"
#include "ember/gpu/GPUInterface.h"
#include "ember/util/ResourceManager.h"

namespace ember::graphics {

    class Renderer {
    public:
        static void init(ecs::World& world);

        Renderer(
            std::shared_ptr<const gpu::GPUDevice> gpu_device,
            std::unique_ptr<Window> window
        );
        ~Renderer();

        inline Window* window() { return m_window.get(); }
        inline const Window* window() const { return m_window.get(); }

        void render(ecs::World& world);

        // Draw Commands
        void draw_mesh(uint64_t mesh_id, const glm::mat4& transform);

    private:
        std::shared_ptr<const gpu::GPUDevice> m_gpu_device;
        std::unique_ptr<gpu::GPUInterface> m_gpu_interface;
        std::unique_ptr<Window> m_window;

        std::vector<std::unique_ptr<Renderpass>> m_renderpasses;
        Renderpass::RenderpassObjects m_renderpass_objects;
        util::ResourceManager<Mesh> m_mesh_manager;
    };
    static_assert(ecs::System<Renderer>);

}