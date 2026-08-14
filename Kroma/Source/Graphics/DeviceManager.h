// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include "Core/Base.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Window;

typedef struct DeviceManager
{
    SDL_GPUDevice *device;
    bool is_initialized;
} DeviceManager;

KR_API int device_manager_init(DeviceManager *manager, SDL_GPUShaderFormat shader_formats, bool debug_mode);
KR_API void device_manager_shutdown(DeviceManager *manager);

KR_API SDL_GPUDevice *device_manager_get_device(const DeviceManager *manager);
KR_API int device_manager_claim_window(DeviceManager *manager, struct Window *window);
KR_API void device_manager_release_window(DeviceManager *manager, struct Window *window);

KR_API DeviceManager *device_manager_get_instance(void);
KR_API int device_manager_init_global(SDL_GPUShaderFormat shader_formats, bool debug_mode);
KR_API void device_manager_shutdown_global(void);

#ifdef __cplusplus
}
#endif

#endif
