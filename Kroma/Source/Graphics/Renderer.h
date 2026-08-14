// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _RENDERER_H
#define _RENDERER_H

#include "Core/Math.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// Maximum number of quads in a single batch
#define MAX_QUADS 1000000
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

typedef struct Vertex2D
{
    Vector2f position;
    Vector4f color;
} Vertex2D;

typedef struct BatchRenderer2D
{
    SDL_GPUDevice *device;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    
    Vertex2D *vertices;
    uint32_t vertex_count;
    uint32_t quad_count;
    
    bool in_batch;
} BatchRenderer2D;

KR_API bool batch_renderer_2d_init(BatchRenderer2D *renderer, SDL_GPUDevice *device);
KR_API void batch_renderer_2d_destroy(BatchRenderer2D *renderer);
KR_API void batch_renderer_2d_begin(BatchRenderer2D *renderer);
KR_API void batch_renderer_2d_add_quad(BatchRenderer2D *renderer, Vector2f position, Vector2f size, Vector4f color);
KR_API void batch_renderer_2d_end(BatchRenderer2D *renderer);
KR_API void batch_renderer_2d_draw(BatchRenderer2D *renderer, SDL_GPURenderPass *render_pass);

#ifdef __cplusplus
}
#endif


#endif