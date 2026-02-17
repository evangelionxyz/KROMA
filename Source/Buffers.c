#include "Buffers.h"

bool upload_to_gpu_buffer(SDL_GPUDevice *device, SDL_GPUBuffer *dst_buffer, const void *data, uint32_t size, uint32_t offset)
{
    if (!device || !dst_buffer || !data || size == 0)
    {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = size;

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create transfer buffer: %s", SDL_GetError());
        return false;
    }

    void *mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!mapped_data)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_memcpy(mapped_data, data, size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer *upload_cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!upload_cmd)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
    if (!copy_pass)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin copy pass: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUTransferBufferLocation src_location = {0};
    src_location.transfer_buffer = transfer_buffer;
    src_location.offset = 0;

    SDL_GPUBufferRegion dst_region = {0};
    dst_region.buffer = dst_buffer;
    dst_region.offset = offset;
    dst_region.size = size;

    SDL_UploadToGPUBuffer(copy_pass, &src_location, &dst_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    
    if (!SDL_SubmitGPUCommandBuffer(upload_cmd))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to submit command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return true;
}

VertexBuffer vertex_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_vertices)
{
    VertexBuffer vertex_buffer = {0};
    vertex_buffer.num_vertices = num_vertices;

    SDL_GPUBufferCreateInfo buffer_info = {0};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = data_size;

    vertex_buffer.buffer = SDL_CreateGPUBuffer(device, &buffer_info);

    if (vertex_buffer.buffer && data)
    {
        if (!upload_to_gpu_buffer(device, vertex_buffer.buffer, data, data_size, 0))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload vertex buffer data");
        }
    }

    return vertex_buffer;
}

void vertex_buffer_destroy(SDL_GPUDevice *device, VertexBuffer *vertex_buffer)
{
    if (vertex_buffer && vertex_buffer->buffer)
    {
        SDL_ReleaseGPUBuffer(device, vertex_buffer->buffer);
        vertex_buffer->buffer = NULL;
        vertex_buffer->num_vertices = 0;
    }
}

IndexBuffer index_buffer_create(SDL_GPUDevice *device, const void *data, uint32_t data_size, uint32_t num_indices)
{
    IndexBuffer index_buffer = {0};
    index_buffer.num_indices = num_indices;

    SDL_GPUBufferCreateInfo buffer_info = {0};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info.size = data_size;

    index_buffer.buffer = SDL_CreateGPUBuffer(device, &buffer_info);

    if (index_buffer.buffer && data)
    {
        if (!upload_to_gpu_buffer(device, index_buffer.buffer, data, data_size, 0))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload index buffer data");
        }
    }

    return index_buffer;
}

void index_buffer_destroy(SDL_GPUDevice *device, IndexBuffer *index_buffer)
{
    if (index_buffer && index_buffer->buffer)
    {
        SDL_ReleaseGPUBuffer(device, index_buffer->buffer);
        index_buffer->buffer = NULL;
        index_buffer->num_indices = 0;
    }
}

UniformBuffer uniform_buffer_create(SDL_GPUDevice *device, uint32_t size)
{
    UniformBuffer uniform_buffer = {0};
    uniform_buffer.size = size;

    SDL_GPUBufferCreateInfo buffer_info = {0};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    buffer_info.size = size;

    uniform_buffer.buffer = SDL_CreateGPUBuffer(device, &buffer_info);
    return uniform_buffer;
}

void uniform_buffer_update(SDL_GPUDevice *device, UniformBuffer *uniform_buffer, const void *data, uint32_t size)
{
    if (!uniform_buffer || !uniform_buffer->buffer || !data)
        return;

    if (!upload_to_gpu_buffer(device, uniform_buffer->buffer, data, size, 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to update uniform buffer");
    }
}

void uniform_buffer_destroy(SDL_GPUDevice *device, UniformBuffer *uniform_buffer)
{
    if (uniform_buffer && uniform_buffer->buffer)
    {
        SDL_ReleaseGPUBuffer(device, uniform_buffer->buffer);
        uniform_buffer->buffer = NULL;
        uniform_buffer->size = 0;
    }
}