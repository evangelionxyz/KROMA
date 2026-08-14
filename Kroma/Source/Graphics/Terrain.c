// Copyright (c) 2026 Evangelion Manuhutu

#include "Core/FastNoise.h"

#include "Terrain.h"

#include <SDL3/SDL_log.h>
#include <stdlib.h>

bool terrain_create(Terrain *terrain, SDL_GPUDevice *device, uint32_t grid_width, uint32_t grid_depth, float spacing, float height_scale, int seed)
{
    if (!terrain || !device || grid_width < 2 || grid_depth < 2 || spacing <= 0.0f)
    {
        return false;
    }

    terrain->device = device;
    terrain->grid_width = grid_width;
    terrain->grid_depth = grid_depth;

    const uint32_t vertex_count = grid_width * grid_depth;
    const uint32_t quad_count = (grid_width - 1) * (grid_depth - 1);
    const uint32_t index_count = quad_count * 6;

    TerrainVertex *vertices = (TerrainVertex *)malloc(sizeof(TerrainVertex) * vertex_count);
    uint32_t *indices = (uint32_t *)malloc(sizeof(uint32_t) * index_count);

    if (!vertices || !indices)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate terrain mesh memory");
        free(vertices);
        free(indices);
        return false;
    }

    fnl_state noise = fnlCreateState();
    noise.seed = seed;
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.frequency = 0.04f;
    noise.octaves = 5;
    noise.lacunarity = 2.0f;
    noise.gain = 0.5f;

    const float half_width = ((float)(grid_width - 1) * spacing) * 0.5f;
    const float half_depth = ((float)(grid_depth - 1) * spacing) * 0.5f;

    for (uint32_t z = 0; z < grid_depth; ++z)
    {
        for (uint32_t x = 0; x < grid_width; ++x)
        {
            const uint32_t index = z * grid_width + x;

            const float sample_x = (float)x;
            const float sample_z = (float)z;
            const float noise_value = fnlGetNoise2D(&noise, sample_x, sample_z);
            const float height = noise_value * height_scale;

            vertices[index].position.x = (float)x * spacing - half_width;
            vertices[index].position.y = height;
            vertices[index].position.z = (float)z * spacing - half_depth;

            const float normalized_height = (height / (height_scale * 2.0f)) + 0.5f;
            vertices[index].color.x = 0.15f + normalized_height * 0.35f;
            vertices[index].color.y = 0.25f + normalized_height * 0.55f;
            vertices[index].color.z = 0.12f + normalized_height * 0.25f;
            vertices[index].color.w = 1.0f;
        }
    }

    uint32_t write_index = 0;
    for (uint32_t z = 0; z < grid_depth - 1; ++z)
    {
        for (uint32_t x = 0; x < grid_width - 1; ++x)
        {
            const uint32_t i0 = z * grid_width + x;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + grid_width;
            const uint32_t i3 = i2 + 1;

            indices[write_index++] = i0;
            indices[write_index++] = i2;
            indices[write_index++] = i1;

            indices[write_index++] = i1;
            indices[write_index++] = i2;
            indices[write_index++] = i3;
        }
    }

    terrain->vertex_buffer = vertex_buffer_create(device, vertices, (uint32_t)(vertex_count * sizeof(TerrainVertex)), vertex_count);
    terrain->index_buffer = index_buffer_create(device, indices, (uint32_t)(index_count * sizeof(uint32_t)), index_count);

    free(vertices);
    free(indices);

    if (!terrain->vertex_buffer.buffer || !terrain->index_buffer.buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create terrain GPU buffers");
        terrain_destroy(terrain);
        return false;
    }

    return true;
}

void terrain_destroy(Terrain *terrain)
{
    if (!terrain || !terrain->device)
    {
        return;
    }

    index_buffer_destroy(terrain->device, &terrain->index_buffer);
    vertex_buffer_destroy(terrain->device, &terrain->vertex_buffer);
    terrain->grid_width = 0;
    terrain->grid_depth = 0;
    terrain->device = NULL;
}

void terrain_draw(const Terrain *terrain, SDL_GPURenderPass *render_pass)
{
    if (!terrain || !render_pass || !terrain->vertex_buffer.buffer || !terrain->index_buffer.buffer)
    {
        return;
    }

    SDL_GPUBufferBinding vertex_binding = {0};
    vertex_binding.buffer = terrain->vertex_buffer.buffer;
    vertex_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

    SDL_GPUBufferBinding index_binding = {0};
    index_binding.buffer = terrain->index_buffer.buffer;
    index_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_DrawGPUIndexedPrimitives(render_pass, terrain->index_buffer.num_indices, 1, 0, 0, 0);
}