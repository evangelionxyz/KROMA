#include "Shader.h"
#include <spirv_cross/spirv_cross_c.h>
#include <SDL3/SDL_log.h>

#include <assert.h>

ShaderBinary shader_load_from_binary(const char *filename)
{
    ShaderBinary result = {0};
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open shader binary: %s", filename);
        return result;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return result;
    }

    // get file size
    long length = ftell(file);
    if (length <= 0)
    {
        fclose(file);
        return result;
    }

    // go the begin of memory
    rewind(file);

    uint8_t *buffer = (uint8_t *)malloc((size_t)length);
    if (!buffer)
    {
        fclose(file);
        return result;
    }

    size_t read_size = fread(buffer, 1, (size_t)length, file);
    if (read_size != (size_t)length)
    {
        free(buffer);
        return result;
    }

    result.bytes = buffer;
    result.size = (size_t)length;
    return result;
}

Shader shader_create(SDL_GPUDevice *device, SDL_GPUShaderStage stage, const char *filename, const char *entry_point)
{
    Shader shader = {0};

    ShaderBinary binary_code = shader_load_from_binary(filename);
    if (!binary_code.bytes)
    {
        return shader;
    }
    shader.reflection_info = shader_reflect_spirv((uint32_t *)binary_code.bytes, binary_code.size);
    
    SDL_GPUShaderCreateInfo shader_create_info = {0};
    shader_create_info.code_size = binary_code.size;
    shader_create_info.code = binary_code.bytes;
    shader_create_info.entrypoint = entry_point;
    shader_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shader_create_info.stage = stage;
    shader_create_info.num_samplers = (uint32_t)shader.reflection_info.num_samplers;
    shader_create_info.num_storage_textures = (uint32_t)shader.reflection_info.num_storage_textures;
    shader_create_info.num_storage_buffers = (uint32_t)shader.reflection_info.num_storage_buffers;
    shader_create_info.num_uniform_buffers = (uint32_t)shader.reflection_info.num_uniform_buffers;

    SDL_GPUShader *sdl_shader = SDL_CreateGPUShader(device, &shader_create_info);
    
    // free binary after create shader
    free(binary_code.bytes);

    if (!sdl_shader)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU shaders: %s", filename);
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

ShaderReflectionInfo shader_reflect_spirv(const uint32_t *spirv_data, size_t size_in_bytes)
{
    ShaderReflectionInfo info = {0};
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_resources resources = NULL;
    
    // Create context
    if (spvc_context_create(&context) != SPVC_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create SPIRV-Cross context");
        return info;
    }
    
    // Parse SPIRV
    size_t word_count = size_in_bytes / sizeof(uint32_t);
    if (spvc_context_parse_spirv(context, spirv_data, word_count, &ir) != SPVC_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse SPIRV");
        spvc_context_destroy(context);
        return info;
    }
    
    // Create compiler
    if (spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler) != SPVC_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compiler");
        spvc_context_destroy(context);
        return info;
    }
    
    // Get shader resources
    if (spvc_compiler_create_shader_resources(compiler, &resources) != SPVC_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create shader resources");
        spvc_context_destroy(context);
        return info;
    }

    // Get shader stages
    const spvc_reflected_resource *input_stage_list = NULL;
    size_t input_stage_count = 0;
    spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &input_stage_list, &input_stage_count);
    
    // Uniform buffers
    const spvc_reflected_resource *ubo_list = NULL;
    size_t ubo_count = 0;
    spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &ubo_list, &ubo_count);
    info.num_uniform_buffers = ubo_count;
    
    // Combined image samplers (sampler2D, sampler3D, etc.)
    const spvc_reflected_resource *sampler_list = NULL;
    size_t sampler_count = 0;
    spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, &sampler_list, &sampler_count);
    info.num_samplers = sampler_count;
    
    // Storage textures
    const spvc_reflected_resource *storage_tex_list = NULL;
    size_t storage_tex_count = 0;
    spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &storage_tex_list, &storage_tex_count);
    info.num_storage_textures = storage_tex_count;
    
    // Storage buffers
    const spvc_reflected_resource *storage_buf_list = NULL;
    size_t storage_buf_count = 0;
    spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &storage_buf_list, &storage_buf_count);
    info.num_storage_buffers = storage_buf_count;


    // Vertex inputs (only for vertex shaders)
    info.vertex_attributes = NULL;
    info.vertex_attribute_count = 0;

    if (input_stage_count > 0)
    {
        // Helper struct for sorting by location
        typedef struct VertexInput
        {
            uint32_t location;
            spvc_reflected_resource resource;
        } VertexInput;

        // Allocate array for sorting
        VertexInput *inputs = (VertexInput *)malloc(sizeof(VertexInput) * input_stage_count);
        if (!inputs)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for vertex inputs");
            spvc_context_destroy(context);
            return info;
        }

        // Gather all inputs with their locations
        for (size_t i = 0; i < input_stage_count; ++i)
        {
            inputs[i].resource = input_stage_list[i];
            inputs[i].location = spvc_compiler_get_decoration(compiler, input_stage_list[i].id, SpvDecorationLocation);
        }

        // Simple bubble sort by location (fine for small arrays)
        for (size_t i = 0; i < input_stage_count - 1; ++i)
        {
            for (size_t j = 0; j < input_stage_count - i - 1; ++j)
            {
                if (inputs[j].location > inputs[j + 1].location)
                {
                    VertexInput temp = inputs[j];
                    inputs[j] = inputs[j + 1];
                    inputs[j + 1] = temp;
                }
            }
        }

        // Allocate vertex attributes array
        info.vertex_attributes = (SDL_GPUVertexAttribute *)malloc(sizeof(SDL_GPUVertexAttribute) * input_stage_count);
        if (!info.vertex_attributes)
        {
            free(inputs);
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for vertex attributes");
            spvc_context_destroy(context);
            return info;
        }

        uint32_t offset = 0;
        uint32_t attr_count = 0;

        // Process each input in location order
        for (size_t i = 0; i < input_stage_count; ++i)
        {
            spvc_reflected_resource res = inputs[i].resource;
            spvc_type type_id = spvc_compiler_get_type_handle(compiler, res.type_id);
            
            spvc_basetype base_type = spvc_type_get_basetype(type_id);
            uint32_t vec_size = spvc_type_get_vector_size(type_id);
            uint32_t columns = spvc_type_get_columns(type_id);

            // Map SPIRV type to SDL GPU format
            SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
            uint32_t attribute_size = 0;

            if (base_type == SPVC_BASETYPE_FP32 && columns == 1)
            {
                switch (vec_size)
                {
                    case 1:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
                        attribute_size = 4;
                        break;
                    case 2:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
                        attribute_size = 8;
                        break;
                    case 3:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
                        attribute_size = 12;
                        break;
                    case 4:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
                        attribute_size = 16;
                        break;
                }
            }
            else if (base_type == SPVC_BASETYPE_INT32 && columns == 1)
            {
                switch (vec_size)
                {
                    case 1:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_INT;
                        attribute_size = 4;
                        break;
                    case 2:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_INT2;
                        attribute_size = 8;
                        break;
                    case 3:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_INT3;
                        attribute_size = 12;
                        break;
                    case 4:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_INT4;
                        attribute_size = 16;
                        break;
                }
            }
            else if (base_type == SPVC_BASETYPE_UINT32 && columns == 1)
            {
                switch (vec_size)
                {
                    case 1:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
                        attribute_size = 4;
                        break;
                    case 2:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
                        attribute_size = 8;
                        break;
                    case 3:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_UINT3;
                        attribute_size = 12;
                        break;
                    case 4:
                        format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4;
                        attribute_size = 16;
                        break;
                }
            }

            if (format != SDL_GPU_VERTEXELEMENTFORMAT_INVALID)
            {
                SDL_GPUVertexAttribute *attr = &info.vertex_attributes[attr_count];
                attr->location = inputs[i].location;
                attr->buffer_slot = 0; // Single vertex buffer
                attr->format = format;
                attr->offset = offset;

                offset += attribute_size;
                attr_count++;
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                           "Unsupported vertex attribute format at location %u", inputs[i].location);
            }
        }

        info.vertex_attribute_count = attr_count;
        free(inputs);
    }
    
    spvc_context_destroy(context);
    return info;
}