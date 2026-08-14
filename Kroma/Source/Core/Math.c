// Copyright (c) 2026 Evangelion Manuhutu

#include "Math.h"
#include "Camera.h"
#include <math.h>

Vector2f get_normalized_device_coordinate(float point_x, float point_y, float size_x, float size_y)
{
    // Normalize the coordinate to [-1, 1]
    const float ndc_x = (2.0f * point_x) / size_x - 1.0f;
    const float ndc_y = (2.0f * point_y) / size_y - 1.0f;
    return (Vector2f){ .x = ndc_x, .y = ndc_y };
}

Vector3f get_world_coordinate(Camera *camera, float point_x, float point_y)
{
    const Vector2f ndc = get_normalized_device_coordinate(point_x, point_y,
        (float)camera->viewport_size.x, (float)camera->viewport_size.y);

    mat4 view_projection, inverted_view_projection;
    glm_mat4_mul(camera->projection, camera->view, view_projection);
    glm_mat4_inv(view_projection, inverted_view_projection);

    vec4 near_vec = {ndc.x, ndc.y, 0.0f, 1.0f};
    vec4 far_vec = {ndc.x, ndc.y, 1.0f, 1.0f};
    vec4 world_near_4, world_far_4;

    glm_mat4_mulv(inverted_view_projection, near_vec, world_near_4);
    glm_mat4_mulv(inverted_view_projection, far_vec, world_far_4);

    if (world_near_4[3] != 0.0f)
        glm_vec4_divs(world_near_4, world_near_4[3], world_near_4);
    if (world_far_4[3] != 0.0f)
        glm_vec4_divs(world_far_4, world_far_4[3], world_far_4);

    vec3 world_near = {world_near_4[0], world_near_4[1], world_near_4[2]};
    vec3 world_far = {world_far_4[0], world_far_4[1], world_far_4[2]};

    // Calculate the ray direction
    vec3 dir;
    glm_vec3_sub(world_far, world_near, dir);

    // Intersect with Z=0 plane (standard for 2D particles in 3D world)
    // Ray: P = world_near + t * dir
    // We want P.z = 0  => world_near.z + t * dir.z = 0
    float t = -world_near[2] / dir[2];
    
    vec3 intersection;
    glm_vec3_scale(dir, t, intersection);
    glm_vec3_add(world_near, intersection, intersection);

    return (Vector3f){ 
        .x = intersection[0], 
        .y = intersection[1], 
        .z = intersection[2]
    };
}