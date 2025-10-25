#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Eigen/Dense>
#include <SDL3/SDL.h>

#include "ember/graphics/ArcballCamera.h"
#include "ember/graphics/Window.h"
#include "ember/gpu/Renderer.h"
#include "ember/gpu/VulkanHelpers.h"

using namespace ember::gpu;
using namespace ember::graphics;

class MeshViewer {
public:
    MeshViewer(const char* model_path);
    ~MeshViewer();

    void run();

private:
    std::shared_ptr<const VulkanInstance> m_vkinstance;
    std::shared_ptr<const GraphicsDevice> m_graphics_device;
    std::unique_ptr<Window> m_window;
    Renderer* m_renderer;

    std::array<ShaderModule*, 2> m_shaders;
    DescriptorSetArray<DescriptorSetBlueprint*> m_descriptor_set_blueprints;
    Pipeline* m_pipeline;
    Buffer* m_vertex_buffer, *m_index_buffer, *m_uniform_buffer;
    vk::DescriptorSet m_ubo;
    uint32_t m_index_count;

    Assimp::Importer m_assimp;
    const aiScene* m_scene;
    ArcballCamera m_camera;
    struct ModelViewProjection {
        Eigen::Affine3f model;
        Eigen::Affine3f view;
        Eigen::Matrix4f proj;
    } m_model_mvp;

    void create_window();
    void create_graphics_pipeline();
    void upload_model_data(const char* model_path);
    void update_model_mvp();
};