#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Eigen/Dense>
#include <SDL3/SDL.h>

#include "ember/ecs/Entity.h"
#include "ember/ecs/World.h"
#include "ember/graphics/MeshRenderSystem.h"
#include "ember/graphics/Window.h"

using namespace ember;

class MeshViewer {
public:
    MeshViewer(const char* model_path);
    ~MeshViewer();

    void run();

private:
    std::shared_ptr<const gpu::VulkanInstance> m_vkinstance;
    std::shared_ptr<const gpu::GraphicsDevice> m_graphics_device;
    std::unique_ptr<graphics::Window> m_window;
    ecs::World m_world;
    graphics::MeshRenderSystem m_mesh_renderer;

    ecs::Entity m_scene;
    ecs::Entity m_camera;
    std::vector<ecs::Entity> m_meshes;

    void create_window();
    void load_scene(const char* scene_path);
    void create_camera();
};