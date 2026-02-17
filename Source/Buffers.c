#include "Buffers.h"

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
        SDL_GPUTransferBufferCreateInfo transfer_info = {0};
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_info.size = data_size;

        SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
        if (transfer_buffer)
        {
            void *mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            if (mapped_data)
            {
                SDL_memcpy(mapped_data, data, data_size);
                SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

                SDL_GPUCommandBuffer *upload_cmd = SDL_AcquireGPUCommandBuffer(device);
                SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd);

                SDL_GPUTransferBufferLocation src_location = {0};
                src_location.transfer_buffer = transfer_buffer;
                src_location.offset = 0;

                SDL_GPUBufferRegion dst_region = {0};
                dst_region.buffer = vertex_buffer.buffer;
                dst_region.offset = 0;
                dst_region.size = data_size;

                SDL_UploadToGPUBuffer(copy_pass, &src_location, &dst_region, false);
                SDL_EndGPUCopyPass(copy_pass);
                SDL_SubmitGPUCommandBuffer(upload_cmd);
            }
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
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
        SDL_GPUTransferBufferCreateInfo transfer_info = {0};
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_info.size = data_size;

        SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
        if (transfer_buffer)
        {
            void *mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            if (mapped_data)
            {
                SDL_memcpy(mapped_data, data, data_size);
                SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

                SDL_GPUCommandBuffer *upload_cmd = SDL_AcquireGPUCommandBuffer(device);
                SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd);

                SDL_GPUTransferBufferLocation src_location = {0};
                src_location.transfer_buffer = transfer_buffer;
                src_location.offset = 0;

                SDL_GPUBufferRegion dst_region = {0};
                dst_region.buffer = index_buffer.buffer;
                dst_region.offset = 0;
                dst_region.size = data_size;

                SDL_UploadToGPUBuffer(copy_pass, &src_location, &dst_region, false);
                SDL_EndGPUCopyPass(copy_pass);
                SDL_SubmitGPUCommandBuffer(upload_cmd);
            }
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
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