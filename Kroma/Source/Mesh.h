#ifndef _MESH_H
#define _MESH_H

#include <stdint.h>
#include "cglm/vec3.h"

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
    uint32_t vertexCount;
    uint32_t indexCount;
} Mesh;

#endif
