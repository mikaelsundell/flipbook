#version 440
layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, uv_coord);
}
