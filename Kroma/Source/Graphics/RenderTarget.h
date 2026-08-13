#ifndef _RENDER_TARGET_H
#define _RENDER_TARGET_H

#include <SDL3/SDL_gpu.h>

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

RenderTarget *render_target_create(SDL_GPUDevice *device, RenderTargetDescription *desc);
void render_target_destroy(RenderTarget *render_target);

#endif
