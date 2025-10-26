#pragma once

#include "MeshComponent.h"
#include "SceneComponent.h"
#include "CameraComponent.h"

#include "Renderer.h"
#include "ember/ecs/System.h"
#include "ember/ecs/World.h"
#include "ember/gpu/CommandRecorder.h"
#include "ember/gpu/GraphicsInterface.h"

namespace ember::graphics {

    class MeshRenderSystem {
    public:
        static void init(ecs::World& world);

        MeshRenderSystem() = default;
        MeshRenderSystem(Renderer* window);

        void pre_render(ecs::World& world, Renderer* window);
        void render(ecs::World& world, Renderer* window);

    private:
        std::array<gpu::ShaderModule*, 2> m_shaders;
        gpu::DescriptorSetArray<gpu::DescriptorSetBlueprint*> m_descriptor_set_blueprints;
        gpu::Pipeline* m_pipeline;
        gpu::Buffer* m_vertex_buffer, *m_index_buffer, *m_uniform_buffer, *m_uniform_staging_buffer;
        vk::DescriptorSet m_ubo;
    };
    static_assert(ecs::System<MeshRenderSystem>);


}
