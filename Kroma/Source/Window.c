#include "Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

int window_create(Window *window, const char *title, int width, int height, SDL_WindowFlags flags)
{
    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window pointer is NULL");
        return KR_FAILURE;
    }

    SDL_WindowFlags window_flags = flags != 0 ? flags : SDL_WINDOW_RESIZABLE;

    window->handle = SDL_CreateWindow(title, width, height, window_flags);
    if (!window->handle)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window: %s", SDL_GetError());
        return KR_FAILURE;
    }

    window->id = SDL_GetWindowID(window->handle);
    window->width = width;
    window->height = height;
    window->swapchain_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    window->is_open = true;

    return KR_SUCCESS;
}

void window_destroy(Window *window)
{
    if (!window)
        return;

    if (window->handle)
    {
        SDL_DestroyWindow(window->handle);
        window->handle = NULL;
    }

    window->id = 0;
    window->width = 0;
    window->height = 0;
    window->is_open = false;
}

bool window_handle_event(Window *window, const SDL_Event *event)
{
    if (!window || !window->handle || !event)
        return false;

    switch (event->type)
    {
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            if (event->window.windowID == window->id)
            {
                window->width = event->window.data1;
                window->height = event->window.data2;
                return true;
            }
            break;
        }

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            if (event->window.windowID == window->id)
            {
                window->is_open = false;
                return true;
            }
            break;
        }

        case SDL_EVENT_WINDOW_DESTROYED:
        {
            if (event->window.windowID == window->id)
            {
                window->handle = NULL;
                window->is_open = false;
                return true;
            }
            break;
        }

        default:
            break;
    }

    return false;
}

bool window_is_open(const Window *window)
{
    return window != NULL && window->is_open && window->handle != NULL;
}