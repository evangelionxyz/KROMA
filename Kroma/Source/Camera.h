#ifndef _CAMERA_H
#define _CAMERA_H

#include <stdbool.h>

#include "Math.h"

#include "cglm/cglm.h"

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

void camera_init(Camera *camera, Vector3f position, float yaw_degrees, float pitch_degrees, int viewport_width, int viewport_height, float fov_radians);
void camera_set_viewport(Camera *camera, int width, int height);
void camera_process_keyboard(Camera *camera, const bool *keyboard_state, float delta_time);
void camera_process_mouse(Camera *camera, float delta_x, float delta_y);
void camera_update_matrices(Camera *camera, mat4 view_projection);

#endif