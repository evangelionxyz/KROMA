// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _GPU_TEXTURE_H
#define _GPU_TEXTURE_H

#include "Core/Base.h"

#include <SDL3/SDL_gpu.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GPUTexture
{
    SDL_GPUTexture *texture;
    SDL_GPUSampler *sampler;
    uint32_t width;
    uint32_t height;
} GPUTexture;

// Load a texture from an image file (PNG, JPG, BMP, TGA, etc.)
KR_API GPUTexture gpu_texture_create_from_file(SDL_GPUDevice *device, const char *filepath);

// Load a texture from raw file data in memory
KR_API GPUTexture gpu_texture_create_from_memory(SDL_GPUDevice *device, const unsigned char *data, uint32_t size);

// Create a 1x1 white texture (fallback for missing maps)
KR_API GPUTexture gpu_texture_create_default_white(SDL_GPUDevice *device);

// Destroy texture and sampler
KR_API void gpu_texture_destroy(SDL_GPUDevice *device, GPUTexture *texture);

#ifdef __cplusplus
}
#endif

#endif
