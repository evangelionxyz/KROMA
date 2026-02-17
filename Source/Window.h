#ifndef _WINDOW_H
#define _WINDOW_H

#include "Base.h"

#include <SDL3/SDL.h>

typedef struct Window
{
    SDL_Window *handle;
    SDL_GPUDevice *device;
    SDL_GPUTextureFormat swapchain_format;
    int width;
    int height;
} Window;

int create_window(Window *window, const char *title, int width, int height);
int create_gpu_device(Window *window);
void destroy_gpu_device(Window *window);
void destroy_window(Window *window);

#endif