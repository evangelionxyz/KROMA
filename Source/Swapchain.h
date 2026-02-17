#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

#include "Window.h"
#include <stdint.h>

typedef struct Swapchain
{
    SDL_GPUTexture *texture;
    uint32_t width;
    uint32_t height;
} Swapchain;

int swapchain_acquire(SDL_GPUCommandBuffer *cmd, Window *window, Swapchain *swapchain);

#endif