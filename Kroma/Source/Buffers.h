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

typedef struct UniformBuffer
{
    SDL_GPUBuffer *buffer;
    uint32_t size;
} UniformBuffer;

// Create vertex buffer with data
VertexBuffer vertex_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_vertices);
void vertex_buffer_destroy(SDL_GPUDevice *device, VertexBuffer *vertex_buffer);

// Create index buffer with data
IndexBuffer index_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_indices);
void index_buffer_destroy(SDL_GPUDevice *device, IndexBuffer *index_buffer);

// Create uniform buffer
UniformBuffer uniform_buffer_create(SDL_GPUDevice *device, uint32_t size);
void uniform_buffer_update(SDL_GPUDevice *device, UniformBuffer *uniform_buffer, const void *data, uint32_t size);
void uniform_buffer_destroy(SDL_GPUDevice *device, UniformBuffer *uniform_buffer);

// Helper function to upload data to GPU buffer
bool upload_to_gpu_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *dst_buffer, const void *data, uint32_t size, uint32_t offset);

#endif