#include "SceneComponent.h"

#include <cassert>
#include "Vertex.h"

namespace ember::graphics {

    namespace {
        inline void ai_vector_to_eigen(Eigen::Vector3f& eigen, const aiVector3D& ai) {
            eigen[0] = ai.x;
            eigen[1] = ai.y;
            eigen[2] = ai.z;
        }

        std::vector<Vertex> get_ai_mesh_vertices(const aiMesh* mesh) {
            std::vector<Vertex> vertices(mesh->mNumVertices);

            for (auto v = 0; v < mesh->mNumVertices; v++) {
                auto& vertex = vertices[v];
                ai_vector_to_eigen(vertex.position, mesh->mVertices[v]);
                ai_vector_to_eigen(vertex.normal, mesh->mNormals[v]);
            }

            return vertices;
        }

        std::vector<uint32_t> get_ai_mesh_indices(const aiMesh* mesh) {
            std::vector<uint32_t> indices(mesh->mNumFaces * 3);

            for (auto f = 0; f < mesh->mNumFaces; f++) {
                const auto index = f*3;
                indices[index] = mesh->mFaces[f].mIndices[0];
                indices[index+1] = mesh->mFaces[f].mIndices[1];
                indices[index+2] = mesh->mFaces[f].mIndices[2];
            }

            return indices;
        }
    }

    SceneComponent::SceneComponent(const aiScene* scene, gpu::Renderer* renderer) {
        meshes.resize(scene->mNumMeshes);
        for (auto i = 0; i < scene->mNumMeshes; i++) {
            auto vertices = get_ai_mesh_vertices(scene->mMeshes[i]);
            auto indices = get_ai_mesh_indices(scene->mMeshes[i]);
            meshes[i].index_count = indices.size();

            meshes[i].vertex_buffer = renderer->create_vertex_buffer(vertices.size() * sizeof(Vertex));
            renderer->upload_buffer<Vertex>(meshes[i].vertex_buffer, vertices);

            meshes[i].index_buffer = renderer->create_index_buffer(indices.size() * sizeof(uint32_t));
            renderer->upload_buffer<uint32_t>(meshes[i].index_buffer, indices);
        }
    }

}