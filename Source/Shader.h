#ifndef _SHADER_H
#define _SHADER_H

#include <SDL3/SDL_gpu.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct ShaderReflectionInfo {
    size_t num_uniform_buffers;
    size_t num_samplers;
    size_t num_storage_textures;
    size_t num_storage_buffers;

    SDL_GPUVertexAttribute *vertex_attributes;
    uint32_t vertex_attribute_count;
} ShaderReflectionInfo;

typedef struct ShaderBinary
{
    uint8_t *bytes;
    size_t size;
} ShaderBinary;

typedef struct Shader
{
    SDL_GPUShader *handle;
    ShaderReflectionInfo reflection_info;
} Shader;

ShaderBinary shader_load_from_binary(const char *filename);
Shader shader_create(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const char *filename, const char *entry_point);
void shader_release(SDL_GPUDevice *device, Shader *shader);
ShaderReflectionInfo shader_reflect_spirv(const uint32_t *spirv_data, size_t size_in_bytes);

#endif