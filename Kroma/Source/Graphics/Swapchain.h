// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

#include "Window.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Swapchain
{
    SDL_GPUTexture *texture;
    uint32_t width;
    uint32_t height;
} Swapchain;

KR_API int swapchain_acquire(SDL_GPUCommandBuffer *cmd, Window *window, Swapchain *swapchain);

#ifdef __cplusplus
}
#endif

#endif