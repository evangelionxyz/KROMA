// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "GPUBuffers.h"
#include "Core/Math.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TerrainVertex
{
    Vector3f position;
    Vector4f color;
} TerrainVertex;

typedef struct Terrain
{
    SDL_GPUDevice *device;
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
    uint32_t grid_width;
    uint32_t grid_depth;
} Terrain;

KR_API bool terrain_create(Terrain *terrain, SDL_GPUDevice *device, uint32_t grid_width, uint32_t grid_depth, float spacing, float height_scale, int seed);
KR_API void terrain_destroy(Terrain *terrain);
KR_API void terrain_draw(const Terrain *terrain, SDL_GPURenderPass *render_pass);

#ifdef __cplusplus
}
#endif

#endif