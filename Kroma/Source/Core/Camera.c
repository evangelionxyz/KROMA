// Copyright (c) 2026 Evangelion Manuhutu

#include "Camera.h"

#include <SDL3/SDL_scancode.h>
#include <assert.h>

#include "cglm/clipspace/persp_rh_zo.h"
#include "cglm/clipspace/view_rh_zo.h"

static void camera_update_vectors(Camera *camera)
{
    vec3 forward =
    {
        cosf(camera->yaw) * cosf(camera->pitch),
        sinf(camera->pitch),
        sinf(camera->yaw) * cosf(camera->pitch)
    };

    glm_normalize_to(forward, camera->front);
    glm_cross(camera->front, camera->world_up, camera->right);
    glm_normalize(camera->right);
    glm_cross(camera->right, camera->front, camera->up);
    glm_normalize(camera->up);
}

void camera_init(Camera *camera, Vector3f position, float yaw_degrees, float pitch_degrees, int viewport_width, int viewport_height, float fov_radians)
{
    assert(viewport_width > 0 && viewport_height > 0 && "Please insert a valid viewport size");
    
    if (!camera)
        return;

    camera->position[0] = position.x;
    camera->position[1] = position.y;
    camera->position[2] = position.z;

    camera->world_up[0] = 0.0f;
    camera->world_up[1] = 1.0f;
    camera->world_up[2] = 0.0f;

    camera->yaw = glm_rad(yaw_degrees);
    camera->pitch = glm_rad(pitch_degrees);

    camera->field_of_view = fov_radians;
    camera->near_plane = 0.1f;
    camera->far_plane = 500.0f;

    camera->movement_speed = 10.0f;
    camera->look_sensitivity = 0.0025f;

    camera_set_viewport(camera, viewport_width, viewport_height);
    camera_update_vectors(camera);
}

void camera_set_viewport(Camera *camera, int width, int height)
{
    if (!camera || width < 1 || height < 1)
        return;

    camera->viewport_size.x = width;
    camera->viewport_size.y = height;
}

void camera_process_keyboard(Camera *camera, const bool *keyboard_state, float delta_time)
{
    if (!camera || !keyboard_state || delta_time <= 0.0f)
        return;

    const float velocity = camera->movement_speed * delta_time;

    if (keyboard_state[SDL_SCANCODE_W])
    {
        glm_vec3_muladds(camera->front, velocity, camera->position);
    }
    if (keyboard_state[SDL_SCANCODE_S])
    {
        glm_vec3_muladds(camera->front, -velocity, camera->position);
    }
    if (keyboard_state[SDL_SCANCODE_D])
    {
        glm_vec3_muladds(camera->right, velocity, camera->position);
    }
    if (keyboard_state[SDL_SCANCODE_A])
    {
        glm_vec3_muladds(camera->right, -velocity, camera->position);
    }
    if (keyboard_state[SDL_SCANCODE_E] || keyboard_state[SDL_SCANCODE_SPACE])
    {
        camera->position[1] += velocity;
    }
    if (keyboard_state[SDL_SCANCODE_Q] || keyboard_state[SDL_SCANCODE_LCTRL])
    {
        camera->position[1] -= velocity;
    }
}

void camera_process_mouse(Camera *camera, float delta_x, float delta_y)
{
    if (!camera)
        return;

    camera->yaw += delta_x * camera->look_sensitivity;
    camera->pitch -= delta_y * camera->look_sensitivity;

    const float pitch_limit = glm_rad(89.0f);
    if (camera->pitch > pitch_limit)
    {
        camera->pitch = pitch_limit;
    }
    else if (camera->pitch < -pitch_limit)
    {
        camera->pitch = -pitch_limit;
    }

    camera_update_vectors(camera);
}

void camera_update_matrices(Camera *camera, mat4 view_projection)
{
    if (!camera)
        return;

    const float aspect = (float)camera->viewport_size.x / (float)camera->viewport_size.y;
    glm_perspective_rh_zo(camera->field_of_view, aspect, camera->near_plane, camera->far_plane, camera->projection);
    camera->projection[1][1] *= -1.0f; // Invert Y for Vulkan NDC clip space

    vec3 target;
    glm_vec3_add(camera->position, camera->front, target);
    glm_lookat_rh_zo(camera->position, target, camera->up, camera->view);

    glm_mat4_mul(camera->projection, camera->view, view_projection);
}
