#include "Swapchain.h"

#include <SDL3/SDL_log.h>

int swapchain_acquire(SDL_GPUCommandBuffer *cmd, Window *window, Swapchain *swapchain)
{
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window->handle, &swapchain->texture,
        &swapchain->width, &swapchain->height))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire swapchain texture: %s", SDL_GetError());
        return FAILURE;
    }
    return SUCCESS;
}