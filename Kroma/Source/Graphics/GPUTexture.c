// Copyright (c) 2026 Evangelion Manuhutu

#include "GPUTexture.h"

#include <SDL3/SDL_log.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Helper: upload RGBA8 pixel data to a GPU texture via transfer buffer
static bool upload_texture_data(SDL_GPUDevice *device, SDL_GPUTexture *texture,
    const unsigned char *pixels, uint32_t width, uint32_t height)
{
    const uint32_t data_size = width * height * 4;

    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = data_size;

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to create transfer buffer: %s", SDL_GetError());
        return false;
    }

    void *mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!mapped)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_memcpy(mapped, pixels, data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to acquire command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);
    if (!copy_pass)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to begin copy pass: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUTextureTransferInfo src = {0};
    src.transfer_buffer = transfer_buffer;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {0};
    dst.texture = texture;
    dst.w = width;
    dst.h = height;
    dst.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(cmd))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to submit command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return true;
}

// Helper: create a GPU texture and sampler from raw RGBA8 pixel data
static GPUTexture gpu_texture_create_from_pixels(SDL_GPUDevice *device,
    const unsigned char *pixels, uint32_t width, uint32_t height)
{
    GPUTexture result = {0};

    SDL_GPUTextureCreateInfo tex_info = {0};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex_info.width = width;
    tex_info.height = height;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    result.texture = SDL_CreateGPUTexture(device, &tex_info);
    if (!result.texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to create texture: %s", SDL_GetError());
        return result;
    }

    result.width = width;
    result.height = height;

    // Upload pixel data
    if (!upload_texture_data(device, result.texture, pixels, width, height))
    {
        SDL_ReleaseGPUTexture(device, result.texture);
        result.texture = NULL;
        return result;
    }

    // Create sampler (linear filtering, repeat wrapping)
    SDL_GPUSamplerCreateInfo sampler_info = {0};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    result.sampler = SDL_CreateGPUSampler(device, &sampler_info);
    if (!result.sampler)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to create sampler: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device, result.texture);
        result.texture = NULL;
        return result;
    }

    return result;
}

GPUTexture gpu_texture_create_from_file(SDL_GPUDevice *device, const char *filepath)
{
    GPUTexture result = {0};

    if (!device || !filepath)
    {
        return result;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(0); // glTF textures are top-down
    unsigned char *pixels = stbi_load(filepath, &width, &height, &channels, 4); // Force RGBA
    if (!pixels)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to load image '%s': %s", filepath, stbi_failure_reason());
        return result;
    }

    result = gpu_texture_create_from_pixels(device, pixels, (uint32_t)width, (uint32_t)height);
    stbi_image_free(pixels);

    if (result.texture)
    {
        SDL_Log("GPUTexture: Loaded '%s' (%dx%d)", filepath, width, height);
    }

    return result;
}

GPUTexture gpu_texture_create_from_memory(SDL_GPUDevice *device, const unsigned char *data, uint32_t size)
{
    GPUTexture result = {0};

    if (!device || !data || size == 0)
    {
        return result;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(0);
    unsigned char *pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
    if (!pixels)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUTexture: Failed to load image from memory: %s", stbi_failure_reason());
        return result;
    }

    result = gpu_texture_create_from_pixels(device, pixels, (uint32_t)width, (uint32_t)height);
    stbi_image_free(pixels);

    return result;
}

GPUTexture gpu_texture_create_default_white(SDL_GPUDevice *device)
{
    // 1x1 white pixel (RGBA)
    const unsigned char white_pixel[4] = { 255, 255, 255, 255 };
    return gpu_texture_create_from_pixels(device, white_pixel, 1, 1);
}

void gpu_texture_destroy(SDL_GPUDevice *device, GPUTexture *texture)
{
    if (!texture) return;

    if (texture->sampler)
    {
        SDL_ReleaseGPUSampler(device, texture->sampler);
        texture->sampler = NULL;
    }

    if (texture->texture)
    {
        SDL_ReleaseGPUTexture(device, texture->texture);
        texture->texture = NULL;
    }

    texture->width = 0;
    texture->height = 0;
}
