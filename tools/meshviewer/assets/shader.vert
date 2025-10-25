#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    frag_color = abs(norm);
}
