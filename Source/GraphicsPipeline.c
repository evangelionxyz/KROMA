#include "GraphicsPipeline.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

SDL_GPUGraphicsPipeline *graphics_pipeline_create(SDL_GPUDevice *device, GraphicsPipelineDescription *desc)
{
    SDL_GPUColorTargetDescription color_target_description = {0};
    color_target_description.format = desc->format;
    color_target_description.blend_state.enable_blend = false;
    color_target_description.blend_state.enable_color_write_mask = true;
    color_target_description.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                                            SDL_GPU_COLORCOMPONENT_G |
                                                            SDL_GPU_COLORCOMPONENT_B |
                                                            SDL_GPU_COLORCOMPONENT_A;

    SDL_GPUGraphicsPipelineTargetInfo target_info = {0};
    target_info.color_target_descriptions = &color_target_description;
    target_info.num_color_targets = 1;
    target_info.has_depth_stencil_target = false;

    SDL_GPURasterizerState rasterizer = {0};
    rasterizer.fill_mode = desc->fill_mode;
    rasterizer.cull_mode = desc->cull_mode;
    rasterizer.front_face = desc->front_face;
    rasterizer.enable_depth_bias = false;
    rasterizer.enable_depth_clip = false;

    SDL_GPUMultisampleState multisample = {0};
    multisample.sample_count = SDL_GPU_SAMPLECOUNT_1;
    multisample.sample_mask = 0;
    multisample.enable_mask = false;

    SDL_GPUDepthStencilState depth_stencil = {0};
    depth_stencil.compare_op = desc->compare_op;
    depth_stencil.enable_depth_test = desc->enable_depth_test;
    depth_stencil.enable_depth_write = desc->enable_depth_write;
    depth_stencil.enable_stencil_test = false;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.vertex_shader = desc->vertex_shader;
    pipeline_info.fragment_shader = desc->fragment_shader;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = desc->vertex_buffer_descriptions;
    pipeline_info.vertex_input_state.num_vertex_buffers = desc->num_vertex_buffers;
    pipeline_info.vertex_input_state.vertex_attributes = desc->vertex_attributes;
    pipeline_info.vertex_input_state.num_vertex_attributes = desc->num_vertex_attributes;
    pipeline_info.primitive_type = desc->primitive_type;
    pipeline_info.rasterizer_state = rasterizer;
    pipeline_info.multisample_state = multisample;
    pipeline_info.depth_stencil_state = depth_stencil;
    pipeline_info.target_info = target_info;
    pipeline_info.props = 0;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    if (!pipeline)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pipeline: %s", SDL_GetError());
    }
    return pipeline;
}

void graphics_pipeline_destroy(SDL_GPUDevice *device, SDL_GPUGraphicsPipeline *pipeline)
{
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}
