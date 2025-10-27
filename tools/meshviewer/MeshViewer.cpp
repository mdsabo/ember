#include "MeshViewer.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <numbers>

#include "ember/ecs/TransformComponent.h"
#include "ember/util/Log.h"

#define ASSET_PATH(x) (std::filesystem::path(MESHVIEWER_ASSET_PATH x))


void MeshViewer::create_window() {
    m_vkinstance = gpu::VulkanInstance::create({
        .features {
            .vk_layer_validation = true,
            // .vk_layer_api_dump = true
        }
    });
    auto window = std::make_unique<graphics::Window>(
        m_vkinstance,
        graphics::WindowSettings {
            .title = "SceneViewer",
            .width = 1920,
            .height = 1080,
            .flags = SDL_WINDOW_HIDDEN
        }
    );
    m_graphics_device = gpu::GPUDevice::create(m_vkinstance, window->surface());
    m_renderer = std::make_unique<graphics::Renderer>(m_graphics_device, std::move(window));
}

void MeshViewer::create_camera() {
    m_camera = m_world.create_entity();

    auto& transforms = m_world.write_component<ecs::TransformComponent>();
    transforms.at(m_camera).transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(1.5, 1.5, 1.5));

    auto& cameras = m_world.write_component<graphics::CameraComponent>();
    const auto swapchain_extent = m_renderer->get_swapchain_extent();
    cameras.insert(m_camera, graphics::CameraComponent{
        .focal_point = ecs::WORLD_ORIGIN_ENTITY,
        .projection = glm::infinitePerspective(
            glm::radians(60.0f),
            float(swapchain_extent.width)/swapchain_extent.height,
            0.1f
        ),
        .viewport = vk::Viewport{
            .x = 0.0,
            .y = 0.0,
            .width = float(swapchain_extent.width),
            .height = float(swapchain_extent.height),
            .minDepth = 0.0,
            .maxDepth = 1.0
        },
    });
}

void MeshViewer::load_scene(const char* path) {
    m_scene = m_world.create_entity();
    Assimp::Importer ai_importer;
    const auto ai_scene = ai_importer.ReadFile(
        path,
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenUVCoords
        | aiProcess_GenNormals
        | aiProcess_CalcTangentSpace
        | aiProcess_OptimizeMeshes
        | aiProcess_OptimizeGraph
        | aiProcess_FixInfacingNormals
        | aiProcess_SplitLargeMeshes
    );

    auto& scenes = m_world.write_component<graphics::SceneComponent>();
    scenes.insert(m_scene, graphics::SceneComponent(ai_scene, m_renderer->get_renderer()));

    auto& meshes = m_world.write_component<graphics::MeshComponent>();
    auto& transforms = m_world.write_component<ecs::TransformComponent>();
    m_meshes = std::vector<ecs::Entity>(scenes.at(m_scene).meshes.size());
    info(MESHVIEWER_LOG, "Found {} meshes in {}", m_meshes.size(), path);
    for (size_t i = 0; i < m_meshes.size(); i++) {
        m_meshes[i] = m_world.create_entity();
        meshes.insert(m_meshes[i], graphics::MeshComponent{
            .scene = m_scene,
            .mesh_id = i
        });

        auto& tx = transforms.at(m_meshes[i]).transform;
        tx = glm::rotate(tx, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        tx = glm::rotate(tx, glm::radians(-90.0f), glm::vec3(0.0, 0.0, 1.0));
    }
}

MeshViewer::MeshViewer(const char* scene_path) {
    create_window();

    graphics::MeshRenderSystem::init(m_world);
    m_mesh_renderer = graphics::MeshRenderSystem(m_renderer.get());

    create_camera();
    load_scene(scene_path);
}

MeshViewer::~MeshViewer() {

}

void MeshViewer::update_model() {
    static auto last_time = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
    last_time = currentTime;

    auto& transforms = m_world.write_component<ecs::TransformComponent>();
    for (const auto mesh : m_meshes) {
        auto& tx = transforms.at(mesh);
        tx.transform = glm::rotate<float>(
            tx.transform,
            glm::radians(18.0) * time,
            glm::vec3(0.0, 1.0, 0.0)
        );
    }
}

void MeshViewer::run() {
    m_renderer->window()->set_visible(true);

    SDL_Event e;
    SDL_zero(e);
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = true;
        }

        //update_model();

        m_mesh_renderer.pre_render(m_world, m_renderer.get());

        auto command_buffer = m_renderer->begin_rendering();
        m_mesh_renderer.render(m_world, m_renderer.get());
        m_renderer->end_rendering();
        m_renderer->present_frame();
    }
}