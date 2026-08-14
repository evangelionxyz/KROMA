// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _MESH_H
#define _MESH_H

#include <stdint.h>
#include <stdbool.h>

#include "cglm/vec3.h"
#include "cglm/mat4.h"

#include "GPUBuffers.h"
#include "Material.h"

#ifdef __cplusplus
extern "C" {
#endif

// PBR vertex with position, normal, UV, tangent
typedef struct PBRVertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec4 tangent;  // xyz = tangent direction, w = handedness
} PBRVertex;

// Legacy mesh vertex (kept for backward compatibility)
typedef struct MeshVertex
{
    vec3 position;
    vec3 color;
    vec2 uv;
} MeshVertex;

// A single sub-mesh (one draw call, one material)
typedef struct SubMesh
{
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
    uint32_t material_index;  // Index into StaticMesh.materials[]
} SubMesh;

// A complete loaded model
typedef struct StaticMesh
{
    SDL_GPUDevice *device;

    SubMesh *submeshes;
    uint32_t submesh_count;

    PBRMaterial *materials;
    uint32_t material_count;

    mat4 transform;  // Model matrix
} StaticMesh;

// Legacy mesh (kept for backward compatibility)
typedef struct Mesh
{
    MeshVertex* vertices;
    uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
} Mesh;

// Load a static mesh from a glTF/glb file
KR_API bool static_mesh_load_gltf(StaticMesh *mesh, SDL_GPUDevice *device, const char *filepath);

// Destroy all GPU resources held by the static mesh
KR_API void static_mesh_destroy(StaticMesh *mesh);

// Draw all sub-meshes (binds vertex/index buffers and material textures per sub-mesh)
KR_API void static_mesh_draw(const StaticMesh *mesh, SDL_GPURenderPass *render_pass);

#ifdef __cplusplus
}
#endif

#endif
