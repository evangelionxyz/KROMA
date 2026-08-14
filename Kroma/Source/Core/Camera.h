// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _CAMERA_H
#define _CAMERA_H

#include "Math.h"
#include "cglm/cglm.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CameraLens
{
    bool enable_bloom;
    bool enable_dof;
} CameraLens;

typedef struct Camera
{
    vec3 position;
    vec3 front;
    vec3 right;
    vec3 up;
    vec3 world_up;

    float yaw;
    float pitch;

    // FOV in radians
    float field_of_view;
    float near_plane;
    float far_plane;

    float movement_speed;
    float look_sensitivity;

    Vector2i viewport_size;

    mat4 view;
    mat4 projection;
} Camera;

KR_API void camera_init(Camera *camera, Vector3f position, float yaw_degrees, float pitch_degrees, int viewport_width, int viewport_height, float fov_radians);
KR_API void camera_set_viewport(Camera *camera, int width, int height);
KR_API void camera_process_keyboard(Camera *camera, const bool *keyboard_state, float delta_time);
KR_API void camera_process_mouse(Camera *camera, float delta_x, float delta_y);
KR_API void camera_update_matrices(Camera *camera, mat4 view_projection);

#ifdef __cplusplus
}
#endif

#endif