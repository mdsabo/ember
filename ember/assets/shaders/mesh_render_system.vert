#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec3 frag_color;

layout(std140, binding = 0) uniform UBO {
    mat4 camera;
};

layout(push_constant) uniform Model {
    mat4 transform;
};

void main() {
    gl_Position = camera * transform * vec4(pos.x, pos.y, pos.z, 1.0);
    frag_color = abs(norm);
}
