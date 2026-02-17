#include "Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

int create_window(Window *window, const char *title, int width, int height)
{
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

    window->handle = SDL_CreateWindow(title, width, height, window_flags);
    if (!window->handle)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return FAILURE;
    }
    
    window->width = width;
    window->height = height;

    return SUCCESS;
}

int create_gpu_device(Window *window)
{
    window->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (!window->device)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU device: %s", SDL_GetError());
        SDL_DestroyWindow(window->handle);
        SDL_Quit();
        return FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(window->device, window->handle))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to claim window for GPU device: %s", SDL_GetError());
        SDL_DestroyGPUDevice(window->device);
        SDL_DestroyWindow(window->handle);
        SDL_Quit();
        return FAILURE;
    }

    SDL_SetGPUSwapchainParameters(window->device, window->handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    window->swapchain_format = SDL_GetGPUSwapchainTextureFormat(window->device, window->handle);

    return SUCCESS;
}

void destroy_gpu_device(Window *window)
{
    SDL_ReleaseWindowFromGPUDevice(window->device, window->handle);
    SDL_DestroyGPUDevice(window->device);
}

void destroy_window(Window *window)
{
    SDL_DestroyWindow(window->handle);
}