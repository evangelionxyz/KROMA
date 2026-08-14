// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _MATH_H
#define _MATH_H

#include "Base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Camera Camera;

// FLOAT
typedef struct Vector2f
{
    float x, y;
} Vector2f;

typedef struct Vector3f
{
    float x, y, z;
} Vector3f;

typedef struct Vector4f
{
    float x, y, z, w;
} Vector4f;


// INTEGER
typedef struct Vector2i
{
    int x, y;
} Vector2i;

typedef struct Vector3i
{
    int x, y, z;
} Vector3i;

typedef struct Vector4i
{
    int x, y, z, w;
} Vector4i;

KR_API Vector2f get_normalized_device_coordinate(float point_x, float point_y, float size_x, float size_y);
KR_API Vector3f get_world_coordinate(Camera *camera, float point_x, float point_y);

#ifdef __cplusplus
}
#endif

#endif