// Copyright (c) 2026 Evangelion Manuhutu

#ifndef _GRAPHICS_PIPELINE
#define _GRAPHICS_PIPELINE

#include "Core/Base.h"

#include "Shader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GraphicsPipelineDescription
{
    SDL_GPUPrimitiveType primitive_type;
    SDL_GPUFrontFace front_face;
    SDL_GPUTextureFormat format;
    SDL_GPUFillMode fill_mode;
    SDL_GPUCullMode cull_mode;
    SDL_GPUCompareOp compare_op;
    
    Shader *vertex_shader;
    Shader *fragment_shader;

    bool enable_depth_test;
    bool enable_depth_write;
    bool enable_blend;
    SDL_GPUTextureFormat depth_stencil_format;
    
    // Vertex input (optional)
    SDL_GPUVertexBufferDescription *vertex_buffer_descriptions;
    Uint32 num_vertex_buffers;
} GraphicsPipelineDescription;

KR_API SDL_GPUGraphicsPipeline *graphics_pipeline_create(SDL_GPUDevice *device, GraphicsPipelineDescription *desc);
KR_API void graphics_pipeline_destroy(SDL_GPUDevice *device, SDL_GPUGraphicsPipeline *pipeline);

#ifdef __cplusplus
}
#endif

#endif