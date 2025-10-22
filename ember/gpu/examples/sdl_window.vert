#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec3 color;

void main() {
    color = vec3(255, 0, 0);
}
