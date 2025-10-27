#pragma once

#include <glm/glm.hpp>

namespace ember::graphics {

    struct Mesh {
        std::vector<glm::vec3> positions;

        struct VertexData {
            glm::vec3 normal;
            glm::vec3 uv;
            glm::vec3 tangent;
        };
        std::vector<VertexData> data;

        std::vector<uint32_t> indices;

        // VertexBuffers, buffer offsets, etc
        // IndexBuffer, buffer offset, etc
    };

}