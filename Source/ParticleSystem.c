#include "ParticleSystem.h"
#include "Shader.h"
#include "Buffers.h"
#include <stdlib.h>
#include <string.h>

bool particle_emitter_create(ParticleEmitter *emitter, SDL_GPUDevice *device, Vector2f position, uint32_t max_particles)
{
    if (!emitter || !device || max_particles == 0 || max_particles > MAX_PARTICLES)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid particle emitter parameters");
        return false;
    }
    
    memset(emitter, 0, sizeof(ParticleEmitter));
    emitter->device = device;
    emitter->particle_count = max_particles;
    emitter->active = true;
    
    // Initialize emitter data
    emitter->emitter_data.position = position;
    emitter->emitter_data.particle_count = max_particles;
    emitter->emitter_data.gravity = -9.8f;
    emitter->emitter_data.damping = 0.1f;
    emitter->emitter_data.delta_time = 0.0f;
    
    // Allocate CPU-side particle array
    emitter->particles = (Particle *)calloc(max_particles, sizeof(Particle));
    if (!emitter->particles)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate particle memory");
        return false;
    }
    
    // Initialize particles
    for (uint32_t i = 0; i < max_particles; i++)
    {
        emitter->particles[i].position = position;
        emitter->particles[i].velocity = (Vector2f){0.0f, 0.0f};
        emitter->particles[i].color = (Vector4f){1.0f, 1.0f, 1.0f, 1.0f};
        emitter->particles[i].lifetime = 0.0f;
        emitter->particles[i].size = 0.05f;
    }
    
    // Create GPU particle buffer
    SDL_GPUBufferCreateInfo particle_buffer_info = {0};
    particle_buffer_info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
    particle_buffer_info.size = max_particles * sizeof(Particle);
    
    emitter->particle_buffer = SDL_CreateGPUBuffer(device, &particle_buffer_info);
    if (!emitter->particle_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create particle GPU buffer: %s", SDL_GetError());
        free(emitter->particles);
        return false;
    }
    
    // Upload initial particle data
    if (!upload_to_gpu_buffer(device, emitter->particle_buffer, emitter->particles, 
                               max_particles * sizeof(Particle), 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload initial particle data");
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create emitter buffer (readonly)
    SDL_GPUBufferCreateInfo emitter_buffer_info = {0};
    emitter_buffer_info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
    emitter_buffer_info.size = sizeof(EmitterData);
    
    emitter->emitter_buffer = SDL_CreateGPUBuffer(device, &emitter_buffer_info);
    if (!emitter->emitter_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create emitter GPU buffer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create compute shader
    ShaderBinary compute_binary = shader_load_from_binary("Resources/Shaders/particles.comp.spv");
    
    if (!compute_binary.bytes)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load particle compute shader");
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create compute pipeline
    SDL_GPUComputePipelineCreateInfo compute_pipeline_info = {0};
    compute_pipeline_info.code = compute_binary.bytes;
    compute_pipeline_info.code_size = compute_binary.size;
    compute_pipeline_info.entrypoint = "main";
    compute_pipeline_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    compute_pipeline_info.num_samplers = 0;
    compute_pipeline_info.num_readonly_storage_textures = 0;
    compute_pipeline_info.num_readonly_storage_buffers = 1;    // emitter buffer (binding 0, via SDL_BindGPUComputeStorageBuffers)
    compute_pipeline_info.num_readwrite_storage_textures = 0;
    compute_pipeline_info.num_readwrite_storage_buffers = 1;   // particle buffer (binding 0, via SDL_BeginGPUComputePass)
    compute_pipeline_info.num_uniform_buffers = 0;
    compute_pipeline_info.threadcount_x = 64;                  // must match shader local_size_x
    compute_pipeline_info.threadcount_y = 1;
    compute_pipeline_info.threadcount_z = 1;
    compute_pipeline_info.props = 0;
    
    emitter->compute_pipeline = SDL_CreateGPUComputePipeline(device, &compute_pipeline_info);
    free(compute_binary.bytes);

    if (!emitter->compute_pipeline)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute pipeline: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    SDL_Log("Particle emitter created with %u particles", max_particles);
    return true;
}

void particle_emitter_destroy(ParticleEmitter *emitter)
{
    if (!emitter)
    {
        return;
    }
    
    if (emitter->compute_pipeline)
    {
        SDL_ReleaseGPUComputePipeline(emitter->device, emitter->compute_pipeline);
        emitter->compute_pipeline = NULL;
    }
    
    if (emitter->emitter_buffer)
    {
        SDL_ReleaseGPUBuffer(emitter->device, emitter->emitter_buffer);
        emitter->emitter_buffer = NULL;
    }
    
    if (emitter->particle_buffer)
    {
        SDL_ReleaseGPUBuffer(emitter->device, emitter->particle_buffer);
        emitter->particle_buffer = NULL;
    }
    
    if (emitter->particles)
    {
        free(emitter->particles);
        emitter->particles = NULL;
    }
    
    emitter->device = NULL;
    emitter->particle_count = 0;
    emitter->active = false;
}

void particle_emitter_update(ParticleEmitter *emitter, float delta_time)
{
    if (!emitter || !emitter->active)
    {
        return;
    }
    
    // Update emitter data
    emitter->emitter_data.delta_time = delta_time;
    
    // Upload emitter data to GPU
    if (!upload_to_gpu_buffer(emitter->device, emitter->emitter_buffer, &emitter->emitter_data, sizeof(EmitterData), 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to upload emitter data");
        return;
    }
    
    // Create command buffer for compute dispatch
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(emitter->device);
    if (!cmd)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire command buffer for particle update");
        return;
    }
    
    // Setup readwrite storage buffer binding for particle buffer
    SDL_GPUStorageBufferReadWriteBinding readwrite_binding = {0};
    readwrite_binding.buffer = emitter->particle_buffer;
    readwrite_binding.cycle = true;
    
    // Begin compute pass with readwrite buffer binding
    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &readwrite_binding, 1);
    if (!compute_pass)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin compute pass");
        return;
    }
    
    // Bind compute pipeline
    SDL_BindGPUComputePipeline(compute_pass, emitter->compute_pipeline);
    
    // Bind readonly storage buffer (emitter buffer)
    SDL_GPUBuffer *readonly_buffers[1] = { emitter->emitter_buffer };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, readonly_buffers, 1);
    
    // Dispatch compute shader (64 threads per workgroup, as defined in shader)
    uint32_t workgroup_count = (emitter->particle_count + 63) / 64;
    SDL_DispatchGPUCompute(compute_pass, workgroup_count, 1, 1);
    
    SDL_EndGPUComputePass(compute_pass);
    
    // Submit command buffer
    if (!SDL_SubmitGPUCommandBuffer(cmd))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to submit compute command buffer");
    }
}

void particle_emitter_render(ParticleEmitter *emitter, BatchRenderer2D *batch_renderer)
{
    if (!emitter || !batch_renderer || !emitter->active)
    {
        return;
    }
    
    // Download particle data from GPU to CPU
    // Note: In a real implementation, you might want to use a readback buffer
    // For now, we'll use a simple approach
    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size = emitter->particle_count * sizeof(Particle);
    
    SDL_GPUTransferBuffer *download_buffer = SDL_CreateGPUTransferBuffer(emitter->device, &transfer_info);
    if (!download_buffer)
    {
        return;
    }
    
    // Create download command
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(emitter->device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);
    
    SDL_GPUBufferRegion src_region = {0};
    src_region.buffer = emitter->particle_buffer;
    src_region.offset = 0;
    src_region.size = emitter->particle_count * sizeof(Particle);
    
    SDL_GPUTransferBufferLocation dst_location = {0};
    dst_location.transfer_buffer = download_buffer;
    dst_location.offset = 0;
    
    SDL_DownloadFromGPUBuffer(copy_pass, &src_region, &dst_location);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    
    // Wait for transfer to complete (blocking, but ensures we have the data)
    SDL_WaitForGPUIdle(emitter->device);
    
    // Map and read particle data
    void *mapped_data = SDL_MapGPUTransferBuffer(emitter->device, download_buffer, false);
    if (mapped_data)
    {
        memcpy(emitter->particles, mapped_data, emitter->particle_count * sizeof(Particle));
        SDL_UnmapGPUTransferBuffer(emitter->device, download_buffer);
        
        // Add particles to batch renderer
        for (uint32_t i = 0; i < emitter->particle_count; i++)
        {
            Particle *p = &emitter->particles[i];
            
            // Only render alive particles
            if (p->lifetime > 0.0f)
            {
                batch_renderer_2d_add_quad(batch_renderer, 
                                           p->position, 
                                           (Vector2f){p->size, p->size},
                                           p->color);
            }
        }
    }
    
    SDL_ReleaseGPUTransferBuffer(emitter->device, download_buffer);
}

void particle_emitter_set_position(ParticleEmitter *emitter, Vector2f position)
{
    if (emitter)
    {
        emitter->emitter_data.position = position;
    }
}

void particle_emitter_set_gravity(ParticleEmitter *emitter, float gravity)
{
    if (emitter)
    {
        emitter->emitter_data.gravity = gravity;
    }
}

void particle_emitter_set_damping(ParticleEmitter *emitter, float damping)
{
    if (emitter)
    {
        emitter->emitter_data.damping = damping;
    }
}
