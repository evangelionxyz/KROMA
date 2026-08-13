#ifndef _SHADER_H
#define _SHADER_H

#include <SDL3/SDL_gpu.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct ShaderReflectionInfo {
    uint64_t num_uniform_buffers;
    uint64_t num_samplers;
    uint64_t num_storage_textures;
    uint64_t num_storage_buffers;

    SDL_GPUVertexAttribute *vertex_attributes;
    uint32_t vertex_attribute_count;
} ShaderReflectionInfo;

typedef struct Shader
{
    SDL_GPUShader *handle;
    ShaderReflectionInfo reflection_info;
} Shader;

Shader shader_create(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const char *filepath, const char *entry_point, bool force_recompile);
void shader_release(SDL_GPUDevice *device, Shader *shader);
ShaderReflectionInfo shader_reflect_spirv(SDL_GPUShaderStage stage, const uint32_t *data, uint64_t data_size);

int shader_load_or_compile_binary(SDL_GPUShaderStage stage, const char *filepath, unsigned char **out_data, uint64_t *out_size, const char *entry_point, bool force_recompile);
int shader_load_or_compile_compute_binary(const char *filepath, unsigned char **out_data, uint64_t *out_size, const char *entry_point, bool force_recompile);
int shader_load_from_binary(const char* filepath, unsigned char** outData, uint64_t* out_size);
uint32_t shader_calculate_vertex_stride(ShaderReflectionInfo *reflection_info);

#endif