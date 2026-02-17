#ifndef _BUFFERS_H
#define _BUFFERS_H

#include <SDL3/SDL.h>
#include <stdint.h>

typedef struct VertexBuffer
{
    SDL_GPUBuffer *buffer;
    uint32_t num_vertices;
} VertexBuffer;

typedef struct IndexBuffer
{
    SDL_GPUBuffer *buffer;
    uint32_t num_indices;
} IndexBuffer;

// Create vertex buffer with data
VertexBuffer vertex_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_vertices);
void vertex_buffer_destroy(SDL_GPUDevice *device, VertexBuffer *vertex_buffer);

// Create index buffer with data
IndexBuffer index_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_indices);
void index_buffer_destroy(SDL_GPUDevice *device, IndexBuffer *index_buffer);

#endif