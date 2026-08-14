// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _ARENA_H
#define _ARENA_H

#include "Base.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Arena
{
    void **data;
    size_t cap;
} Arena;

static inline Arena *arena_create(size_t cap)
{
    assert(cap > 0 && "Arena capacity must be greater than 0");

    Arena *arena = (Arena *)malloc(sizeof(Arena));
    if (!arena) return NULL;

    arena->cap = cap;
    arena->data = (void **)malloc(arena->cap * sizeof(void *));

    return arena;
}

static inline void arena_free(Arena *arena)
{
    if (arena == NULL)
        return;

    free(arena->data);
    free(arena);
}

#ifdef __cplusplus
}
#endif

#endif