// Copyright (c) 2026 Evangelion Manuhutu
#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "Core/Math.h"

#ifdef __cplusplus
extern "C" {
#endif

// PBR Material
typedef struct MaterialGPUData
{
    size_t diffuse_index;
    size_t emissive_index;
    size_t metallic_roughness_index;
    size_t occlussion_index;

    Vector4f base_color_factor;
    Vector3f emissive_factor;
    
    float metallic_factor;
    float roughness_factor;
    float occlusion_factor;
} MaterialGPUData;

typedef struct PBRMaterial
{
    size_t material_instance_index;
} PBRMaterial;

#ifdef __cplusplus
}
#endif

#endif
