// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _WINDOW_H
#define _WINDOW_H

#include "Core/Base.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Window
{
    SDL_Window *handle;
    SDL_WindowID id;
    SDL_GPUTextureFormat swapchain_format;
    int width;
    int height;
    bool is_open;
} Window;

KR_API int window_create(Window *window, const char *title, int width, int height, SDL_WindowFlags flags);
KR_API void window_destroy(Window *window);
KR_API bool window_handle_event(Window *window, const SDL_Event *event);
KR_API bool window_is_open(const Window *window);

#ifdef __cplusplus
}
#endif

#endif // _WINDOW_H