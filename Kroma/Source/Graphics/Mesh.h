// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _MESH_H
#define _MESH_H

#include <stdint.h>
#include "cglm/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MeshVertex
{
    vec3 position;
    vec3 color;
    vec2 uv;
} MeshVertex;

typedef struct Mesh
{
    MeshVertex* vertices;
    uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
} Mesh;

typedef struct MeshInstance
{
    uint32_t meesh_instance_index;

} MeshInstance;

#ifdef __cplusplus
}
#endif

#endif
