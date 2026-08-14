// Copyright (c) 2026 Evangelion Manuhutu

#include "Material.h"

#include <SDL3/SDL_log.h>

PBRMaterial pbr_material_create_default(SDL_GPUDevice *device)
{
    PBRMaterial material = {0};

    // Create white fallback textures for all maps
    material.albedo_map = gpu_texture_create_default_white(device);
    material.normal_map = gpu_texture_create_default_white(device);
    material.metallic_roughness_map = gpu_texture_create_default_white(device);
    material.occlusion_map = gpu_texture_create_default_white(device);

    // Default factors
    material.base_color_factor = (Vector4f){1.0f, 1.0f, 1.0f, 1.0f};
    material.metallic_factor = 1.0f;
    material.roughness_factor = 1.0f;
    material.occlusion_strength = 1.0f;
    material.emissive_factor = (Vector3f){0.0f, 0.0f, 0.0f};

    return material;
}

void pbr_material_destroy(SDL_GPUDevice *device, PBRMaterial *material)
{
    if (!material) return;

    gpu_texture_destroy(device, &material->albedo_map);
    gpu_texture_destroy(device, &material->normal_map);
    gpu_texture_destroy(device, &material->metallic_roughness_map);
    gpu_texture_destroy(device, &material->occlusion_map);
}

void pbr_material_bind(const PBRMaterial *material, SDL_GPURenderPass *render_pass)
{
    if (!material || !render_pass) return;

    SDL_GPUTextureSamplerBinding bindings[4] = {0};

    // Slot 0: Albedo
    bindings[0].texture = material->albedo_map.texture;
    bindings[0].sampler = material->albedo_map.sampler;

    // Slot 1: Normal
    bindings[1].texture = material->normal_map.texture;
    bindings[1].sampler = material->normal_map.sampler;

    // Slot 2: Metallic-Roughness
    bindings[2].texture = material->metallic_roughness_map.texture;
    bindings[2].sampler = material->metallic_roughness_map.sampler;

    // Slot 3: Occlusion
    bindings[3].texture = material->occlusion_map.texture;
    bindings[3].sampler = material->occlusion_map.sampler;

    SDL_BindGPUFragmentSamplers(render_pass, 0, bindings, 4);
}

MaterialGPUData pbr_material_get_gpu_data(const PBRMaterial *material,
    Vector3f camera_pos, Vector3f light_position, float light_intensity, Vector3f light_color)
{
    MaterialGPUData data = {0};

    data.base_color_factor = material->base_color_factor;
    data.emissive_factor = (Vector4f){
        material->emissive_factor.x,
        material->emissive_factor.y,
        material->emissive_factor.z,
        material->occlusion_strength
    };
    data.metallic_roughness = (Vector4f){
        material->metallic_factor,
        material->roughness_factor,
        0.0f, 0.0f
    };
    data.camera_pos = (Vector4f){camera_pos.x, camera_pos.y, camera_pos.z, 0.0f};
    data.light_position = (Vector4f){light_position.x, light_position.y, light_position.z, light_intensity};
    data.light_color = (Vector4f){light_color.x, light_color.y, light_color.z, 0.0f};

    return data;
}
