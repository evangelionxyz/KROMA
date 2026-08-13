#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include "../Base.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

struct Window;

typedef struct DeviceManager
{
    SDL_GPUDevice *device;
    bool is_initialized;
} DeviceManager;

/**
 * @brief Initialize a DeviceManager and create the underlying SDL_GPUDevice.
 * @param manager Pointer to DeviceManager instance.
 * @param shader_formats Supported shader formats (e.g. SDL_GPU_SHADERFORMAT_SPIRV).
 * @param debug_mode Enable graphics validation/debug layer.
 * @return KR_SUCCESS on success, KR_FAILURE on error.
 */
int device_manager_init(DeviceManager *manager, SDL_GPUShaderFormat shader_formats, bool debug_mode);

/**
 * @brief Shutdown a DeviceManager and destroy the underlying SDL_GPUDevice.
 * @param manager Pointer to DeviceManager instance.
 */
void device_manager_shutdown(DeviceManager *manager);

/**
 * @brief Get the underlying SDL_GPUDevice from a DeviceManager.
 */
SDL_GPUDevice *device_manager_get_device(const DeviceManager *manager);

/**
 * @brief Claims a window for rendering with this GPU device, sets default swapchain parameters,
 *        and caches the swapchain format in the Window structure.
 * @param manager Pointer to DeviceManager instance.
 * @param window Pointer to Window instance to claim.
 * @return KR_SUCCESS on success, KR_FAILURE on error.
 */
int device_manager_claim_window(DeviceManager *manager, struct Window *window);

/**
 * @brief Releases a window from the GPU device.
 * @param manager Pointer to DeviceManager instance.
 * @param window Pointer to Window instance to release.
 */
void device_manager_release_window(DeviceManager *manager, struct Window *window);

/**
 * Global default DeviceManager helpers for convenience across engine subsystems.
 */
DeviceManager *device_manager_get_instance(void);
int device_manager_init_global(SDL_GPUShaderFormat shader_formats, bool debug_mode);
void device_manager_shutdown_global(void);

#endif // _DEVICE_MANAGER_H
