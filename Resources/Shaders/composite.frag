#version 450

layout(location = 0) in vec2 frag_texcoord;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D scene_texture;

void main()
{
    out_color = texture(scene_texture, frag_texcoord);
}