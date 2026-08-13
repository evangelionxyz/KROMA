#include "RenderTarget.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

RenderTarget *render_target_create(SDL_GPUDevice *device, RenderTargetDescription *desc)
{
    RenderTarget *rt = (RenderTarget *)malloc(sizeof(RenderTarget));
    if (!rt)
        return NULL;

    memset(rt, 0, sizeof(RenderTarget));
    rt->device = device;
    return rt;
}

void render_target_destroy(RenderTarget *render_target)
{
    if (!render_target)
        return;
    
    if (render_target->device)
    {
        if (render_target->color_textures)
        {
            for (size_t i = 0; i < render_target->color_texture_count; ++i)
            {
                if (render_target->color_textures[i])
                {
                    SDL_ReleaseGPUTexture(render_target->device, render_target->color_textures[i]);
                }
            }
            free(render_target->color_textures);
        }
        if (render_target->depth_texture)
        {
            SDL_ReleaseGPUTexture(render_target->device, render_target->depth_texture);
        }
        if (render_target->sampler)
        {
            SDL_ReleaseGPUSampler(render_target->device, render_target->sampler);
        }
    }

    free(render_target);
}