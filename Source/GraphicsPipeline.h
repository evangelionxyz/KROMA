#ifndef _GRAPHICS_PIPELINE
#define _GRAPHICS_PIPELINE

#include "Shader.h"

typedef struct GraphicsPipelineDescription
{
    SDL_GPUPrimitiveType primitive_type;
    SDL_GPUFrontFace front_face;
    SDL_GPUTextureFormat format;
    SDL_GPUFillMode fill_mode;
    SDL_GPUCullMode cull_mode;
    SDL_GPUCompareOp compare_op;
    SDL_GPUShader *vertex_shader;
    SDL_GPUShader *fragment_shader;
    bool enable_depth_test;
    bool enable_depth_write;
    bool enable_blend;
    
    // Vertex input (optional)
    SDL_GPUVertexBufferDescription *vertex_buffer_descriptions;
    Uint32 num_vertex_buffers;
    SDL_GPUVertexAttribute *vertex_attributes;
    Uint32 num_vertex_attributes;
    
    // Fragment samplers (optional)
    Uint32 num_samplers;
} GraphicsPipelineDescription;

SDL_GPUGraphicsPipeline *graphics_pipeline_create(SDL_GPUDevice *device, GraphicsPipelineDescription *desc);
void graphics_pipeline_destroy(SDL_GPUDevice *device, SDL_GPUGraphicsPipeline *pipeline);

#endif