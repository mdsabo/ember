#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec3 frag_color;

void main() {

    mat3 rotationx;
    rotationx[0] = vec3(1.0, 0.0, 0.0);
    rotationx[1] = vec3(0.0, 0.5, 0.86602540378);
    rotationx[2] = vec3(0.0, -0.86602540378, 0.5);

    mat3 rotationy;
    rotationy[0] = vec3(0.5, 0.0, 0.86602540378);
    rotationy[1] = vec3(0.0, 1.0, 0.0);
    rotationy[2] = vec3(0.86602540378, 0.0, 0.5);

    vec3 newpos = rotationx * rotationy * pos;
    gl_Position = vec4(newpos.x, newpos.y, 0.0, 1.0);
    frag_color = norm;
}
