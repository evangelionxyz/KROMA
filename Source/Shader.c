#include "Base.h"

#include "Shader.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_filesystem.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <Umbra/ShaderCompilerCAPI.h>

static const char *SHADER_CACHE_DIRECTORY = "Resources/Shaders/GLSL/Cache";

static int is_path_separator(char ch)
{
    return ch == '/' || ch == '\\';
}

static int ensure_directory(const char* path)
{
    if (SDL_CreateDirectory(path))
    {
        return 1;
    }
    return 0;
}

static int ensure_directory_recursive(const char* path)
{
    char temp[1024] = {0};
    size_t len = strlen(path);
    if (len >= sizeof(temp))
    {
        return 0;
    }

    snprintf(temp, sizeof(temp), "%s", path);

    for (size_t i = 1; i < len; ++i)
    {
        if (is_path_separator(temp[i]))
        {
            char saved = temp[i];
            temp[i] = '\0';
            if (strlen(temp) > 0)
            {
                ensure_directory(temp);
            }
            temp[i] = saved;
        }
    }

    return ensure_directory(temp);
}

static const char* shader_get_filename_from_path(const char* path)
{
    const char* slash = strrchr(path, '/');
    const char* backslash = strrchr(path, '\\');
    const char* fileName = slash;
    if (backslash != NULL && (fileName == NULL || backslash > fileName))
    {
        fileName = backslash;
    }
    return (fileName == NULL) ? path : (fileName + 1);
}

static bool shader_build_output_path(char* out_path, size_t out_path_size, const char* out_dir, const char* input_path, UMBRA_ShaderPlatformType platform)
{
    const char* file_name = shader_get_filename_from_path(input_path);
    char base_name[256] = {0};
    snprintf(base_name, sizeof(base_name), "%s", file_name);

    char* dot = strrchr(base_name, '.');
    if (dot != NULL)
    {
        *dot = '\0';
    }

    return snprintf(out_path, out_path_size, "%s/%s%s", out_dir, base_name, UMBRA_ShaderPlatformExtension(platform)) > 0;
}

static void print_reflection_summary(const char* label, const UmbraShaderReflectionInfo* reflection)
{
    printf("  [%s Reflection] type=%s, UBO=%zu, Samplers=%zu, StorageTex=%zu, StorageBuf=%zu, Inputs=%zu, Outputs=%zu, PushConstants=%zu\n",
        label,
        UMBRA_GetShaderTypeString(reflection->shaderType),
        reflection->numUniformBuffers,
        reflection->numSamplers,
        reflection->numStorageTextures,
        reflection->numStorageBuffers,
        reflection->numStageInputs,
        reflection->numStageOutputs,
        reflection->numPushConstants);
}

int shader_load_or_compile_binary(SDL_GPUShaderStage stage, const char *filepath, unsigned char **out_data, uint64_t *out_size, const char *entry_point, bool force_recompile)
{
    char binary_filepath[1024];
    UMBRA_ShaderPlatformType platform_type = UMBRA_SHADER_PLATFORM_TYPE_SPIRV;
    UMBRA_ShaderType shader_type = shader_get_umbra_shader_type(stage);

    if (!ensure_directory_recursive(SHADER_CACHE_DIRECTORY))
    {
        printf("Failed to create cache directory: %s\n", SHADER_CACHE_DIRECTORY);
    }

    if (!shader_build_output_path(binary_filepath, sizeof(binary_filepath), SHADER_CACHE_DIRECTORY, filepath, platform_type))
    {
        return KR_FAILURE;
    }

    // Try to load cached binary first
    if (force_recompile == KR_FALSE && shader_load_from_binary(binary_filepath, out_data, out_size))
    {
        printf("Loaded cached shader: %s\n", binary_filepath);
        return KR_SUCCESS;
    }

    // Compile if not found
    UmbraCompileRequest request = {0};
    UMBRA_ResultCode compile_result;

    request.inputPath = filepath;
    request.outputDirectory = SHADER_CACHE_DIRECTORY;
    request.entryPoint = entry_point;
    request.shaderModel = "6_5";
    request.vulkanVersion = "1.3";
    request.platformType = platform_type;
    request.shaderType = shader_type;
    request.optimizationLevel = UMBRA_OPT_LEVEL_3;
    request.tRegShift = 0;
    request.sRegShift = 0;
    request.rRegShift = 0;
    request.uRegShift = 0;

    compile_result = UmbraCompiler_Compile(&request);
    printf("Compile (%s) %s -> %d\n", UMBRA_ShaderPlatformToString(platform_type), filepath, (int)compile_result);
    if (compile_result != UMBRA_RESULT_OK)
    {
        assert(false && "Failed to compile shader");
        return KR_FAILURE;
    }

    if (!shader_load_from_binary(binary_filepath, out_data, out_size))
    {
        printf("  Failed to load compiled output: %s\n", binary_filepath);
        return KR_FAILURE;
    }

    return KR_SUCCESS;
}

int shader_load_or_compile_compute_binary(const char *filepath, unsigned char **out_data, uint64_t *out_size, const char *entry_point, bool force_recompile)
{
    char binary_filepath[1024];
    UMBRA_ShaderPlatformType platform_type = UMBRA_SHADER_PLATFORM_TYPE_SPIRV;
    const UMBRA_ShaderType shader_type = UMBRA_SHADER_TYPE_COMPUTE;

    if (!ensure_directory_recursive(SHADER_CACHE_DIRECTORY))
    {
        printf("Failed to create cache directory: %s\n", SHADER_CACHE_DIRECTORY);
    }

    if (!shader_build_output_path(binary_filepath, sizeof(binary_filepath), SHADER_CACHE_DIRECTORY, filepath, platform_type))
    {
        return KR_FAILURE;
    }

    // Try to load cached binary first
    if (force_recompile == KR_FALSE && shader_load_from_binary(binary_filepath, out_data, out_size))
    {
        printf("Loaded cached shader: %s\n", binary_filepath);
        return KR_SUCCESS;
    }

    // Compile if not found
    UmbraCompileRequest request = {0};
    UMBRA_ResultCode compile_result;

    request.inputPath = filepath;
    request.outputDirectory = SHADER_CACHE_DIRECTORY;
    request.entryPoint = entry_point;
    request.shaderModel = "6_5";
    request.vulkanVersion = "1.3";
    request.platformType = platform_type;
    request.shaderType = shader_type;
    request.optimizationLevel = UMBRA_OPT_LEVEL_3;
    request.tRegShift = 0;
    request.sRegShift = 0;
    request.rRegShift = 0;
    request.uRegShift = 0;

    compile_result = UmbraCompiler_Compile(&request);
    printf("Compile (%s) %s -> %d\n", UMBRA_ShaderPlatformToString(platform_type), filepath, (int)compile_result);
    if (compile_result != UMBRA_RESULT_OK)
    {
        assert(false && "Failed to compile shader");
        return KR_FAILURE;
    }

    if (!shader_load_from_binary(binary_filepath, out_data, out_size))
    {
        printf("  Failed to load compiled output: %s\n", binary_filepath);
        return KR_FAILURE;
    }

    return KR_SUCCESS;
}

int shader_load_from_binary(const char *filepath, unsigned char **out_data, uint64_t *out_size)
{
    FILE* file = fopen(filepath, "rb");
    long fileSize;

    uint64_t readSize;
    unsigned char* data;

    *out_data = NULL;
    *out_size = 0;

    if (file == NULL)
    {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }

    fileSize = ftell(file);
    if (fileSize <= 0)
    {
        fclose(file);
        return 0;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return 0;
    }

    data = (unsigned char*)malloc((uint64_t)fileSize);
    if (data == NULL)
    {
        fclose(file);
        return 0;
    }

    readSize = fread(data, 1, (uint64_t)fileSize, file);
    fclose(file);

    if (readSize != (uint64_t)fileSize)
    {
        free(data);
        return 0;
    }

    *out_data = data;
    *out_size = readSize;
    return 1;
}

uint32_t shader_calculate_vertex_stride(ShaderReflectionInfo *reflection_info)
{
    uint32_t vertex_stride = 0;
    
    // Process attributes ordered by location (0 to 15 max)
    for (uint32_t loc = 0; loc < 16; ++loc)
    {
        for (uint32_t i = 0; i < reflection_info->vertex_attribute_count; ++i)
        {
            SDL_GPUVertexAttribute *attr = &reflection_info->vertex_attributes[i];
            if (attr->location == loc)
            {
                attr->offset = vertex_stride;
                uint32_t attr_size = 0;
                
                // Calculate size based on format
                switch (attr->format)
                {
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT:
                        attr_size = 4;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT2:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT2:
                        attr_size = 8;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT3:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT3:
                        attr_size = 12;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT4:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT4:
                        attr_size = 16;
                        break;
                    default:
                        break;
                }
                vertex_stride += attr_size;
            }
        }
    }

    return vertex_stride;
}

UMBRA_ShaderType shader_get_umbra_shader_type(SDL_GPUShaderStage state)
{
    if (state == SDL_GPU_SHADERSTAGE_VERTEX)
        return UMBRA_SHADER_TYPE_VERTEX;
    else if (state == SDL_GPU_SHADERSTAGE_FRAGMENT)
        return UMBRA_SHADER_TYPE_PIXEL;

    assert(false && "Invalid shader stage");
    return UMBRA_SHADER_TYPE_VERTEX;
}

Shader shader_create(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const char *filepath, const char *entry_point, bool force_recompile)
{
    Shader shader = {0};
    
    unsigned char *shader_data = NULL;
    uint64_t shader_data_size = 0;

    if (shader_load_or_compile_binary(stage, filepath, &shader_data, &shader_data_size, entry_point, force_recompile) == KR_FAILURE)
    {
        assert(false && "failed to compile shader");
        return shader;
    }

    UmbraShaderReflectionInfo reflection_info = {0};
    UMBRA_ResultCode result_code = UmbraCompiler_ReflectSPIRV((const uint32_t *)shader_data, shader_data_size,
        shader_get_umbra_shader_type(stage), &reflection_info);

    shader.reflection_info.num_samplers = (uint32_t)reflection_info.numSamplers;
    shader.reflection_info.num_storage_buffers = (uint32_t)reflection_info.numStorageBuffers;
    shader.reflection_info.num_storage_textures = (uint32_t)reflection_info.numStorageTextures;
    shader.reflection_info.num_uniform_buffers = (uint32_t)reflection_info.numUniformBuffers;
    shader.reflection_info.vertex_attribute_count = (uint32_t)reflection_info.vertexAttributeCount;

    if (shader.reflection_info.vertex_attribute_count > 0)
    {
        shader.reflection_info.vertex_attributes = malloc(sizeof(SDL_GPUVertexAttribute) * shader.reflection_info.vertex_attribute_count);
        for (uint32_t i = 0; i < shader.reflection_info.vertex_attribute_count; ++i)
        {
            shader.reflection_info.vertex_attributes[i].offset = reflection_info.vertexAttributes[i].offset;
            shader.reflection_info.vertex_attributes[i].format = (SDL_GPUVertexElementFormat)reflection_info.vertexAttributes[i].format;
            shader.reflection_info.vertex_attributes[i].buffer_slot = reflection_info.vertexAttributes[i].bufferIndex;
            shader.reflection_info.vertex_attributes[i].location = reflection_info.vertexAttributes[i].location;
        }
    }
    else
    {
        shader.reflection_info.vertex_attributes = NULL;
    }

    print_reflection_summary("SPIRV", &reflection_info);

    UmbraCompiler_FreeReflectionInfo(&reflection_info);

    SDL_GPUShaderCreateInfo shader_create_info = {0};
    shader_create_info.code = shader_data;
    shader_create_info.code_size = shader_data_size;
    shader_create_info.entrypoint = entry_point;
    shader_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shader_create_info.stage = stage;
    shader_create_info.num_samplers = (uint32_t)shader.reflection_info.num_samplers;
    shader_create_info.num_storage_textures = (uint32_t)shader.reflection_info.num_storage_textures;
    shader_create_info.num_storage_buffers = (uint32_t)shader.reflection_info.num_storage_buffers;
    shader_create_info.num_uniform_buffers = (uint32_t)shader.reflection_info.num_uniform_buffers;

    SDL_GPUShader *sdl_shader = SDL_CreateGPUShader(device, &shader_create_info);
    
    // free binary after create shader
    free(shader_data);

    if (!sdl_shader)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU shaders: %s", filepath);
    }
    else
    {
        shader.handle = sdl_shader;
    }

    return shader;
}

void shader_release(SDL_GPUDevice *device, Shader *shader)
{
    if (!shader)
        return;

    if (shader->handle)
    {
        SDL_ReleaseGPUShader(device, shader->handle);
        shader->handle = NULL;
    }

    if (shader->reflection_info.vertex_attributes)
    {
        free(shader->reflection_info.vertex_attributes);
        shader->reflection_info.vertex_attributes = NULL;
        shader->reflection_info.vertex_attribute_count = 0;
    }
}

ShaderReflectionInfo shader_reflect_spirv(SDL_GPUShaderStage stage, const uint32_t *data, uint64_t data_size)
{
    ShaderReflectionInfo info = {0};
    
    UmbraShaderReflectionInfo reflection_info;
    memset(&reflection_info, 0, sizeof(UmbraShaderReflectionInfo));

    if ((data_size % 4) != 0)
    {
        printf("  Reflection failed (SPIRV size must be 4-byte aligned).\n");
    }
    else
    {
        UMBRA_ShaderType shader_type = shader_get_umbra_shader_type(stage);
        UMBRA_ResultCode result = UmbraCompiler_ReflectSPIRV(data, data_size, shader_type, &reflection_info);
        if (result == UMBRA_RESULT_OK)
        {
            print_reflection_summary("SPIRV", &reflection_info);
        }
    }

    // Populate info
    info.vertex_attribute_count = (uint32_t)reflection_info.vertexAttributeCount;
    if (info.vertex_attribute_count > 0)
    {
        info.vertex_attributes = malloc(sizeof(SDL_GPUVertexAttribute) * info.vertex_attribute_count);
        for (uint32_t i = 0; i < info.vertex_attribute_count; ++i)
        {
            info.vertex_attributes[i].offset = reflection_info.vertexAttributes[i].offset;
            info.vertex_attributes[i].format = (SDL_GPUVertexElementFormat)reflection_info.vertexAttributes[i].format;
            info.vertex_attributes[i].buffer_slot = reflection_info.vertexAttributes[i].bufferIndex;
            info.vertex_attributes[i].location = reflection_info.vertexAttributes[i].location;
        }
    }
    else
    {
        info.vertex_attributes = NULL;
    }

    UmbraCompiler_FreeReflectionInfo(&reflection_info);

    return info;
}
