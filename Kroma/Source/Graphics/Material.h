// Copyright (c) 2026 Evangelion Manuhutu
#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "Core/Math.h"
#include "Graphics/GPUTexture.h"

#include <SDL3/SDL_gpu.h>

#ifdef __cplusplus
extern "C" {
#endif

// GPU-side material parameters (must match shader layout)
typedef struct MaterialGPUData
{
    Vector4f base_color_factor;
    Vector4f emissive_factor;       // xyz = emissive, w = occlusion_strength
    Vector4f metallic_roughness;    // x = metallic, y = roughness, zw = padding
    Vector4f camera_pos;            // xyz = camera world position
    Vector4f light_position;        // xyz = position, w = intensity
    Vector4f light_color;           // xyz = color
} MaterialGPUData;

// PBR Material
typedef struct PBRMaterial
{
    // Texture maps
    GPUTexture albedo_map;              // Base color (sRGB)
    GPUTexture normal_map;              // Tangent-space normals
    GPUTexture metallic_roughness_map;  // G=roughness, B=metallic (glTF convention)
    GPUTexture occlusion_map;           // Ambient occlusion (R channel)

    // Scalar factors (fallbacks / multipliers)
    Vector4f base_color_factor;         // Multiplied with albedo texture
    float metallic_factor;              // 0.0 = dielectric, 1.0 = metal
    float roughness_factor;             // 0.0 = smooth, 1.0 = rough
    float occlusion_strength;           // AO multiplier

    Vector3f emissive_factor;           // Emissive color (additive)
} PBRMaterial;

// Create a default PBR material with white fallback textures
KR_API PBRMaterial pbr_material_create_default(SDL_GPUDevice *device);

// Destroy all GPU resources held by the material
KR_API void pbr_material_destroy(SDL_GPUDevice *device, PBRMaterial *material);

// Bind material textures to fragment sampler slots 0-3
KR_API void pbr_material_bind(const PBRMaterial *material, SDL_GPURenderPass *render_pass);

// Fill a MaterialGPUData struct from material parameters + scene info
KR_API MaterialGPUData pbr_material_get_gpu_data(const PBRMaterial *material,
    Vector3f camera_pos, Vector3f light_position, float light_intensity, Vector3f light_color);

#ifdef __cplusplus
}
#endif

#endif
