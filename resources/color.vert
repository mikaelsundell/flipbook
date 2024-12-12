#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) out vec2 uv_coord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main() {
    uv_coord = uv;
    gl_Position = mvp * position;
}
