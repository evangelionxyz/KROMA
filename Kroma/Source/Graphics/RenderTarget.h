// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _RENDER_TARGET_H
#define _RENDER_TARGET_H

#include "Core/Base.h"

#include <SDL3/SDL_gpu.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct RenderTargetDescription
{
    uint32_t width;
    uint32_t height;
} RenderTargetDescription;

typedef struct RenderTarget
{
    SDL_GPUDevice *device;
    SDL_GPUTexture **color_textures;
    SDL_GPUTexture *depth_texture;

    SDL_GPUSampler *sampler;

    size_t color_texture_count;
} RenderTarget;

KR_API RenderTarget *render_target_create(SDL_GPUDevice *device, RenderTargetDescription *desc);
KR_API void render_target_destroy(RenderTarget *render_target);


#ifdef __cplusplus
}
#endif


#endif
