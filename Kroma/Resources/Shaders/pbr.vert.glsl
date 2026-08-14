#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec3 frag_world_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out vec3 frag_tangent;
layout(location = 4) out vec3 frag_bitangent;

// Storage buffer for view-projection (matches existing pattern)
layout(set = 0, binding = 0) buffer ViewProjBuffer
{
    mat4 viewProjection;
};

// Storage buffer for model matrix
layout(set = 0, binding = 1) buffer ModelBuffer
{
    mat4 model;
};

void main()
{
    vec4 world_pos = model * vec4(in_position, 1.0);
    frag_world_pos = world_pos.xyz;

    mat3 normal_matrix = transpose(inverse(mat3(model)));
    frag_normal = normalize(normal_matrix * in_normal);
    frag_uv = in_uv;

    // TBN matrix for normal mapping
    vec3 T = normalize(normal_matrix * in_tangent.xyz);
    vec3 N = frag_normal;
    vec3 B = cross(N, T) * in_tangent.w;

    frag_tangent = T;
    frag_bitangent = B;

    gl_Position = viewProjection * world_pos;
}
