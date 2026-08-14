// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _DYNAMIC_ARRAY_H
#define _DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DynArray
{
    void **data;
    size_t size;
    size_t cap;
} DynArray;

static inline DynArray *da_create(size_t cap)
{
    DynArray *da = (DynArray *)malloc(sizeof(DynArray));
    if (!da) return NULL;

    da->size = 0;
    da->cap = cap > 0 ? cap : 8; // Default to some capacity if 0
    da->data = (void **)malloc(da->cap * sizeof(void *));
    if (!da->data)
    {
        free(da);
        return NULL;
    }
    return da;
}

static inline void da_free(DynArray *da)
{
    if (da == NULL)
        return;

    free(da->data);
    free(da);
}

static inline bool da_push_back(DynArray *da, void *elem)
{
    if (da == NULL)
        return false;

    if (da->size >= da->cap)
    {
        size_t new_cap = da->cap == 0 ? 8 : da->cap * 2;
        void **new_data = (void **)realloc(da->data, new_cap * sizeof(void *));
        if (!new_data) return false;

        da->data = new_data;
        da->cap = new_cap;
    }

    da->data[da->size++] = elem;
    return true;
}

static inline void da_pop_back(DynArray *da)
{
    if (da == NULL || da->data == NULL || da->size == 0)
        return;

    // The user should manage the lifetime of the elements themselves.
    da->data[da->size - 1] = NULL;
    da->size--;
}

static inline bool da_insert_element(DynArray *da, void *elem, size_t index)
{
    if (da == NULL || index > da->size)
        return false;

    if (da->size >= da->cap)
    {
        size_t new_cap = da->cap == 0 ? 8 : da->cap * 2;
        void **new_data = (void **)realloc(da->data, new_cap * sizeof(void *));
        if (!new_data) return false;

        da->data = new_data;
        da->cap = new_cap;
    }

    if (index < da->size)
    {
        memmove(&da->data[index + 1], &da->data[index], (da->size - index) * sizeof(void *));
    }

    da->data[index] = elem;
    da->size++;
    return true;
}

static inline bool da_remove_element(DynArray *da, size_t index)
{
    if (da == NULL || index >= da->size)
        return false;

    const size_t elements_to_move = da->size - index - 1;
    if (elements_to_move > 0)
    {
        memmove(&da->data[index], &da->data[index + 1], elements_to_move * sizeof(void *));
    }

    da->data[--da->size] = NULL;
    return true;
}

void *da_get_element(DynArray *da, size_t index)
{
    if (index >= da->size || da == NULL || da->data == NULL)
        return NULL;

    return da->data[index];
}

#ifdef __cplusplus
}
#endif

#endif