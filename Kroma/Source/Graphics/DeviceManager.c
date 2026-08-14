// Copyright (c) 2026 Evangelion Manuhutu

#include "DeviceManager.h"
#include "Window.h"

#include "Core/Base.h"
#include <SDL3/SDL_log.h>

static DeviceManager s_global_device_manager = {0};

int device_manager_init(DeviceManager *manager, SDL_GPUShaderFormat shader_formats, bool debug_mode)
{
    if (!manager)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "DeviceManager pointer is NULL");
        return KR_FAILURE;
    }

    if (manager->is_initialized && manager->device)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "DeviceManager is already initialized");
        return KR_SUCCESS;
    }

    manager->device = SDL_CreateGPUDevice(shader_formats, debug_mode, NULL);
    if (!manager->device)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create SDL GPU Device: %s", SDL_GetError());
        manager->is_initialized = false;
        return KR_FAILURE;
    }

    manager->is_initialized = true;
    return KR_SUCCESS;
}

void device_manager_shutdown(DeviceManager *manager)
{
    if (!manager || !manager->device)
        return;

    SDL_WaitForGPUIdle(manager->device);
    SDL_DestroyGPUDevice(manager->device);

    manager->device = NULL;
    manager->is_initialized = false;
}

SDL_GPUDevice *device_manager_get_device(const DeviceManager *manager)
{
    if (!manager)
        return NULL;
    return manager->device;
}

int device_manager_claim_window(DeviceManager *manager, struct Window *window)
{
    if (!manager || !manager->device)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot claim window: DeviceManager or GPU device is invalid");
        return KR_FAILURE;
    }

    if (!window || !window->handle)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot claim window: Window or handle is NULL");
        return KR_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(manager->device, window->handle))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Failed to claim window (%u) for GPU device: %s",
            window->id, SDL_GetError());
        
        return KR_FAILURE;
    }

    SDL_SetGPUSwapchainParameters(manager->device, window->handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    window->swapchain_format = SDL_GetGPUSwapchainTextureFormat(manager->device, window->handle);
    return KR_SUCCESS;
}

void device_manager_release_window(DeviceManager *manager, struct Window *window)
{
    if (!manager || !manager->device || !window || !window->handle)
        return;

    SDL_ReleaseWindowFromGPUDevice(manager->device, window->handle);
    window->swapchain_format = SDL_GPU_TEXTUREFORMAT_INVALID;
}

DeviceManager *device_manager_get_instance(void)
{
    return &s_global_device_manager;
}

int device_manager_init_global(SDL_GPUShaderFormat shader_formats, bool debug_mode)
{
    return device_manager_init(&s_global_device_manager, shader_formats, debug_mode);
}

void device_manager_shutdown_global(void)
{
    device_manager_shutdown(&s_global_device_manager);
}

