#ifndef _WINDOW_H
#define _WINDOW_H

#include "Base.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdbool.h>

typedef struct Window
{
    SDL_Window *handle;
    SDL_WindowID id;
    SDL_GPUTextureFormat swapchain_format;
    int width;
    int height;
    bool is_open;
} Window;

int window_create(Window *window, const char *title, int width, int height, SDL_WindowFlags flags);
void window_destroy(Window *window);
bool window_handle_event(Window *window, const SDL_Event *event);
bool window_is_open(const Window *window);

#endif // _WINDOW_H