#ifndef _TERRAIN_H
#define _TERRAIN_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#include "Buffers.h"
#include "Math.h"

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

bool terrain_create(Terrain *terrain, SDL_GPUDevice *device, uint32_t grid_width, uint32_t grid_depth, float spacing, float height_scale, int seed);
void terrain_destroy(Terrain *terrain);
void terrain_draw(const Terrain *terrain, SDL_GPURenderPass *render_pass);

#endif