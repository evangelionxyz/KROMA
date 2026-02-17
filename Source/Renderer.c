#include "Renderer.h"
#include "Buffers.h"
#include <stdlib.h>
#include <string.h>

bool batch_renderer_2d_init(BatchRenderer2D *renderer, SDL_GPUDevice *device)
{
    if (!renderer || !device)
    {
        return false;
    }
    
    memset(renderer, 0, sizeof(BatchRenderer2D));
    renderer->device = device;
    
    // Create vertex buffer (large enough for 1 million quads = 4 million vertices)
    SDL_GPUBufferCreateInfo vertex_buffer_info = {0};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = MAX_VERTICES * sizeof(Vertex2D);
    
    renderer->vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
    if (!renderer->vertex_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex buffer for batch renderer: %s", SDL_GetError());
        return false;
    }
    
    // Create index buffer (1 million quads = 6 million indices)
    SDL_GPUBufferCreateInfo index_buffer_info = {0};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = MAX_INDICES * sizeof(uint32_t);
    
    renderer->index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
    if (!renderer->index_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create index buffer for batch renderer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, renderer->vertex_buffer);
        return false;
    }
    
    // Pre-generate indices (this pattern repeats for all quads)
    uint32_t *indices = (uint32_t *)malloc(MAX_INDICES * sizeof(uint32_t));
    if (!indices)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for indices");
        SDL_ReleaseGPUBuffer(device, renderer->vertex_buffer);
        SDL_ReleaseGPUBuffer(device, renderer->index_buffer);
        return false;
    }
    
    for (uint32_t i = 0; i < MAX_QUADS; i++)
    {
        uint32_t vertex_offset = i * 4;
        uint32_t index_offset = i * 6;
        
        // First triangle (0, 1, 2)
        indices[index_offset + 0] = vertex_offset + 0;
        indices[index_offset + 1] = vertex_offset + 1;
        indices[index_offset + 2] = vertex_offset + 2;
        
        // Second triangle (2, 3, 0)
        indices[index_offset + 3] = vertex_offset + 2;
        indices[index_offset + 4] = vertex_offset + 3;
        indices[index_offset + 5] = vertex_offset + 0;
    }
    
    // Upload indices to GPU (one-time upload since indices never change)
    if (!upload_to_gpu_buffer(device, renderer->index_buffer, indices, MAX_INDICES * sizeof(uint32_t), 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload index buffer for batch renderer");
        free(indices);
        SDL_ReleaseGPUBuffer(device, renderer->vertex_buffer);
        SDL_ReleaseGPUBuffer(device, renderer->index_buffer);
        return false;
    }
    
    free(indices);
    
    // Allocate CPU-side vertex array
    renderer->vertices = (Vertex2D *)malloc(MAX_VERTICES * sizeof(Vertex2D));
    if (!renderer->vertices)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for vertices");
        SDL_ReleaseGPUBuffer(device, renderer->vertex_buffer);
        SDL_ReleaseGPUBuffer(device, renderer->index_buffer);
        return false;
    }
    
    renderer->vertex_count = 0;
    renderer->quad_count = 0;
    renderer->in_batch = false;
    
    SDL_Log("Batch renderer initialized: %u max quads (%u vertices, %u indices)", 
            MAX_QUADS, MAX_VERTICES, MAX_INDICES);
    
    return true;
}

void batch_renderer_2d_destroy(BatchRenderer2D *renderer)
{
    if (!renderer)
    {
        return;
    }
    
    if (renderer->vertices)
    {
        free(renderer->vertices);
        renderer->vertices = NULL;
    }
    
    if (renderer->index_buffer)
    {
        SDL_ReleaseGPUBuffer(renderer->device, renderer->index_buffer);
        renderer->index_buffer = NULL;
    }
    
    if (renderer->vertex_buffer)
    {
        SDL_ReleaseGPUBuffer(renderer->device, renderer->vertex_buffer);
        renderer->vertex_buffer = NULL;
    }
    
    renderer->device = NULL;
    renderer->vertex_count = 0;
    renderer->quad_count = 0;
    renderer->in_batch = false;
}

void batch_renderer_2d_begin(BatchRenderer2D *renderer)
{
    if (!renderer)
    {
        return;
    }
    
    renderer->vertex_count = 0;
    renderer->quad_count = 0;
    renderer->in_batch = true;
}

void batch_renderer_2d_add_quad(BatchRenderer2D *renderer, 
                                 Vector2f position, Vector2f size, 
                                 Vector4f color)
{
    if (!renderer || !renderer->in_batch)
    {
        return;
    }
    
    // Check if we have space for another quad
    if (renderer->quad_count >= MAX_QUADS)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Batch renderer is full, cannot add more quads");
        return;
    }
    
    // Calculate quad corners
    float x0 = position.x - size.x * 0.5f;
    float y0 = position.y - size.y * 0.5f;
    float x1 = position.x + size.x * 0.5f;
    float y1 = position.y + size.y * 0.5f;
    
    // Add 4 vertices for the quad
    uint32_t base_index = renderer->vertex_count;
    
    // Bottom-left
    renderer->vertices[base_index + 0].position.x = x0;
    renderer->vertices[base_index + 0].position.y = y0;
    renderer->vertices[base_index + 0].color = color;
    
    // Bottom-right
    renderer->vertices[base_index + 1].position.x = x1;
    renderer->vertices[base_index + 1].position.y = y0;
    renderer->vertices[base_index + 1].color = color;
    
    // Top-right
    renderer->vertices[base_index + 2].position.x = x1;
    renderer->vertices[base_index + 2].position.y = y1;
    renderer->vertices[base_index + 2].color = color;
    
    // Top-left
    renderer->vertices[base_index + 3].position.x = x0;
    renderer->vertices[base_index + 3].position.y = y1;
    renderer->vertices[base_index + 3].color = color;
    
    renderer->vertex_count += 4;
    renderer->quad_count += 1;
}

void batch_renderer_2d_end(BatchRenderer2D *renderer)
{
    if (!renderer || !renderer->in_batch)
    {
        return;
    }
    
    if (renderer->vertex_count == 0)
    {
        renderer->in_batch = false;
        return;
    }
    
    // Upload vertex data to GPU using helper function
    size_t upload_size = renderer->vertex_count * sizeof(Vertex2D);
    if (!upload_to_gpu_buffer(renderer->device, renderer->vertex_buffer, renderer->vertices, upload_size, 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload batch vertex data");
    }
    
    renderer->in_batch = false;
}

void batch_renderer_2d_draw(BatchRenderer2D *renderer, SDL_GPURenderPass *render_pass)
{
    if (!renderer || !render_pass || renderer->quad_count == 0)
    {
        return;
    }
    
    SDL_GPUBufferBinding vertex_binding = {0};
    vertex_binding.buffer = renderer->vertex_buffer;
    vertex_binding.offset = 0;
    
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
    
    SDL_GPUBufferBinding index_binding = {0};
    index_binding.buffer = renderer->index_buffer;
    index_binding.offset = 0;
    
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    
    uint32_t num_indices = renderer->quad_count * 6;
    SDL_DrawGPUIndexedPrimitives(render_pass, num_indices, 1, 0, 0, 0);
}