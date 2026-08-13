#include "ParticleSystem.h"
#include "Shader.h"
#include "Buffers.h"
#include <stdlib.h>
#include <string.h>

#include "Base.h"

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
        emitter->particles[i].size = 0.5f;
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
    if (!upload_to_gpu_buffer(device, emitter->particle_buffer, emitter->particles, max_particles * sizeof(Particle), 0))
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
    unsigned char *shader_data = NULL;
    uint64_t shader_byte_size = 0;
    KR_RESULT success = shader_load_or_compile_compute_binary("Resources/Shaders/particles.comp.glsl", &shader_data, &shader_byte_size, "main", KR_TRUE);
    
    if (!shader_data || success == KR_FAILURE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load particle compute shader");
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create compute pipeline
    SDL_GPUComputePipelineCreateInfo compute_pipeline_info = {0};
    compute_pipeline_info.code = shader_data;
    compute_pipeline_info.code_size = shader_byte_size;
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
    free(shader_data);
    shader_data = NULL;

    if (!emitter->compute_pipeline)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute pipeline: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create alive particle buffer (for culled particles)
    SDL_GPUBufferCreateInfo alive_buffer_info = {0};
    alive_buffer_info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
    alive_buffer_info.size = max_particles * sizeof(Particle);
    
    emitter->alive_particle_buffer = SDL_CreateGPUBuffer(device, &alive_buffer_info);
    if (!emitter->alive_particle_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create alive particle buffer: %s", SDL_GetError());
        SDL_ReleaseGPUComputePipeline(device, emitter->compute_pipeline);
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create counter buffer
    SDL_GPUBufferCreateInfo counter_buffer_info = {0};
    counter_buffer_info.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
    counter_buffer_info.size = sizeof(uint32_t);
    
    emitter->counter_buffer = SDL_CreateGPUBuffer(device, &counter_buffer_info);
    if (!emitter->counter_buffer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create counter buffer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, emitter->alive_particle_buffer);
        SDL_ReleaseGPUComputePipeline(device, emitter->compute_pipeline);
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Initialize counter to 0
    uint32_t zero = 0;
    if (!upload_to_gpu_buffer(device, emitter->counter_buffer, &zero, sizeof(uint32_t), 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize counter buffer");
        SDL_ReleaseGPUBuffer(device, emitter->counter_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->alive_particle_buffer);
        SDL_ReleaseGPUComputePipeline(device, emitter->compute_pipeline);
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create cull compute shader
    success = shader_load_or_compile_compute_binary("Resources/Shaders/particle_cull.comp.glsl", &shader_data, &shader_byte_size, "main", KR_TRUE);
    
    if (!shader_data || success == KR_FAILURE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load particle cull compute shader");
        SDL_ReleaseGPUBuffer(device, emitter->counter_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->alive_particle_buffer);
        SDL_ReleaseGPUComputePipeline(device, emitter->compute_pipeline);
        SDL_ReleaseGPUBuffer(device, emitter->particle_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->emitter_buffer);
        free(emitter->particles);
        return false;
    }
    
    // Create cull compute pipeline
    SDL_GPUComputePipelineCreateInfo cull_pipeline_info = {0};
    cull_pipeline_info.code = shader_data;
    cull_pipeline_info.code_size = shader_byte_size;
    cull_pipeline_info.entrypoint = "main";
    cull_pipeline_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    cull_pipeline_info.num_samplers = 0;
    cull_pipeline_info.num_readonly_storage_textures = 0;
    cull_pipeline_info.num_readonly_storage_buffers = 1;
    cull_pipeline_info.num_readwrite_storage_textures = 0;
    cull_pipeline_info.num_readwrite_storage_buffers = 2;
    cull_pipeline_info.num_uniform_buffers = 1;
    cull_pipeline_info.threadcount_x = 64;
    cull_pipeline_info.threadcount_y = 1;
    cull_pipeline_info.threadcount_z = 1;
    cull_pipeline_info.props = 0;
    
    emitter->cull_pipeline = SDL_CreateGPUComputePipeline(device, &cull_pipeline_info);
    free(shader_data);
    shader_data = NULL;
    
    if (!emitter->cull_pipeline)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create cull compute pipeline: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, emitter->counter_buffer);
        SDL_ReleaseGPUBuffer(device, emitter->alive_particle_buffer);
        SDL_ReleaseGPUComputePipeline(device, emitter->compute_pipeline);
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
    
    if (emitter->cull_pipeline)
    {
        SDL_ReleaseGPUComputePipeline(emitter->device, emitter->cull_pipeline);
        emitter->cull_pipeline = NULL;
    }
    
    if (emitter->compute_pipeline)
    {
        SDL_ReleaseGPUComputePipeline(emitter->device, emitter->compute_pipeline);
        emitter->compute_pipeline = NULL;
    }
    
    if (emitter->counter_buffer)
    {
        SDL_ReleaseGPUBuffer(emitter->device, emitter->counter_buffer);
        emitter->counter_buffer = NULL;
    }
    
    if (emitter->alive_particle_buffer)
    {
        SDL_ReleaseGPUBuffer(emitter->device, emitter->alive_particle_buffer);
        emitter->alive_particle_buffer = NULL;
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
    SDL_GPUStorageBufferReadWriteBinding readwrite_binding[1] = {0};
    readwrite_binding[0].buffer = emitter->particle_buffer;
    readwrite_binding[0].cycle = false;
    
    // Begin compute pass with readwrite buffer binding
    SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(cmd, NULL, 0, readwrite_binding, ARRAY_SIZE(readwrite_binding));
    if (!compute_pass)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin compute pass");
        return;
    }
    
    // Bind compute pipeline
    SDL_BindGPUComputePipeline(compute_pass, emitter->compute_pipeline);
    
    // Bind readonly storage buffer (emitter buffer)
    SDL_GPUBuffer *readonly_buffers[1] = { emitter->emitter_buffer };
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, readonly_buffers, ARRAY_SIZE(readonly_buffers));
    
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
    
    // Reset counter to 0
    uint32_t zero = 0;
    if (!upload_to_gpu_buffer(emitter->device, emitter->counter_buffer, &zero, sizeof(uint32_t), 0))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reset counter buffer");
        return;
    }
    
    // Run culling compute shader to filter alive particles
    SDL_GPUCommandBuffer *cull_cmd = SDL_AcquireGPUCommandBuffer(emitter->device);
    if (!cull_cmd)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire command buffer for culling");
        return;
    }
    
    // Setup readwrite storage buffer bindings for alive particles and counter
    SDL_GPUStorageBufferReadWriteBinding readwrite_bindings[2] = {0};
    readwrite_bindings[0].buffer = emitter->alive_particle_buffer;
    readwrite_bindings[0].cycle = true;
    readwrite_bindings[1].buffer = emitter->counter_buffer;
    readwrite_bindings[1].cycle = false;
    
    SDL_GPUComputePass *cull_pass = SDL_BeginGPUComputePass(cull_cmd, NULL, 0, readwrite_bindings, ARRAY_SIZE(readwrite_bindings));
    if (!cull_pass)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin cull compute pass");
        return;
    }
    
    SDL_BindGPUComputePipeline(cull_pass, emitter->cull_pipeline);
    
    // Bind readonly storage buffer (particle buffer)
    SDL_GPUBuffer *readonly_buffers[1] = { emitter->particle_buffer };
    SDL_BindGPUComputeStorageBuffers(cull_pass, 0, readonly_buffers, ARRAY_SIZE(readonly_buffers));
    
    // Push constant for particle count
    SDL_PushGPUComputeUniformData(cull_cmd, 0, &emitter->particle_count, sizeof(uint32_t));
    
    // Dispatch culling shader
    uint32_t workgroup_count = (emitter->particle_count + 63) / 64;
    SDL_DispatchGPUCompute(cull_pass, workgroup_count, 1, 1);
    
    SDL_EndGPUComputePass(cull_pass);
    SDL_SubmitGPUCommandBuffer(cull_cmd);
    
    // Download alive count
    SDL_GPUTransferBufferCreateInfo counter_transfer_info = {0};
    counter_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    counter_transfer_info.size = sizeof(uint32_t);
    
    SDL_GPUTransferBuffer *counter_download = SDL_CreateGPUTransferBuffer(emitter->device, &counter_transfer_info);
    if (!counter_download)
    {
        return;
    }
    
    SDL_GPUCommandBuffer *download_cmd = SDL_AcquireGPUCommandBuffer(emitter->device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(download_cmd);
    
    // Download counter
    SDL_GPUBufferRegion counter_region = {0};
    counter_region.buffer = emitter->counter_buffer;
    counter_region.offset = 0;
    counter_region.size = sizeof(uint32_t);
    
    SDL_GPUTransferBufferLocation counter_location = {0};
    counter_location.transfer_buffer = counter_download;
    counter_location.offset = 0;
    
    SDL_DownloadFromGPUBuffer(copy_pass, &counter_region, &counter_location);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(download_cmd);
    
    // Wait for transfer
    SDL_WaitForGPUIdle(emitter->device);
    
    // Read alive count
    uint32_t *count_ptr = (uint32_t *)SDL_MapGPUTransferBuffer(emitter->device, counter_download, false);
    if (!count_ptr)
    {
        SDL_ReleaseGPUTransferBuffer(emitter->device, counter_download);
        return;
    }
    
    uint32_t alive_count = *count_ptr;
    SDL_UnmapGPUTransferBuffer(emitter->device, counter_download);
    SDL_ReleaseGPUTransferBuffer(emitter->device, counter_download);
    
    if (alive_count == 0)
    {
        return; // No particles to render
    }
    
    // Download only alive particles
    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size = alive_count * sizeof(Particle);
    
    SDL_GPUTransferBuffer *download_buffer = SDL_CreateGPUTransferBuffer(emitter->device, &transfer_info);
    if (!download_buffer)
    {
        return;
    }
    
    SDL_GPUCommandBuffer *particle_cmd = SDL_AcquireGPUCommandBuffer(emitter->device);
    SDL_GPUCopyPass *particle_copy = SDL_BeginGPUCopyPass(particle_cmd);
    
    SDL_GPUBufferRegion src_region = {0};
    src_region.buffer = emitter->alive_particle_buffer;
    src_region.offset = 0;
    src_region.size = alive_count * sizeof(Particle);
    
    SDL_GPUTransferBufferLocation dst_location = {0};
    dst_location.transfer_buffer = download_buffer;
    dst_location.offset = 0;
    
    SDL_DownloadFromGPUBuffer(particle_copy, &src_region, &dst_location);
    SDL_EndGPUCopyPass(particle_copy);
    SDL_SubmitGPUCommandBuffer(particle_cmd);
    
    SDL_WaitForGPUIdle(emitter->device);
    
    // Map and read only alive particles
    void *mapped_data = SDL_MapGPUTransferBuffer(emitter->device, download_buffer, false);
    if (mapped_data)
    {
        Particle *alive_particles = (Particle *)mapped_data;
        
        // Add only alive particles to batch renderer (no CPU filtering needed!)
        for (uint32_t i = 0; i < alive_count; i++)
        {
            Particle *p = &alive_particles[i];
            batch_renderer_2d_add_quad(batch_renderer, p->position, (Vector2f){p->size, p->size}, p->color);
        }
        
        SDL_UnmapGPUTransferBuffer(emitter->device, download_buffer);
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
