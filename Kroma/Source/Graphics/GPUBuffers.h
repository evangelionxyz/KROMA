// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _BUFFERS_H
#define _BUFFERS_H

#include "Core/Base.h"

#include <SDL3/SDL.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
KR_API VertexBuffer vertex_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_vertices);
KR_API void vertex_buffer_destroy(SDL_GPUDevice *device, VertexBuffer *vertex_buffer);

// Create index buffer with data
KR_API IndexBuffer index_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_indices);
KR_API void index_buffer_destroy(SDL_GPUDevice *device, IndexBuffer *index_buffer);

// Create uniform buffer
KR_API UniformBuffer uniform_buffer_create(SDL_GPUDevice *device, uint32_t size);
KR_API void uniform_buffer_update(SDL_GPUDevice *device, UniformBuffer *uniform_buffer, const void *data, uint32_t size);
KR_API void uniform_buffer_destroy(SDL_GPUDevice *device, UniformBuffer *uniform_buffer);

// Helper function to upload data to GPU buffer
KR_API bool upload_to_gpu_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *dst_buffer, const void *data, uint32_t size, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif