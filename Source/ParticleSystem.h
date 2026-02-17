#ifndef _PARTICLE_SYSTEM_H
#define _PARTICLE_SYSTEM_H

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include "Math.h"
#include "Renderer.h"

#define MAX_PARTICLES 10000

// Particle structure (must match compute shader layout)
typedef struct Particle
{
    Vector2f position;
    Vector2f velocity;
    Vector4f color;
    float lifetime;
    float size;
    float padding[2];
} Particle;

// Emitter data (must match compute shader uniform layout)
typedef struct EmitterData
{
    Vector2f position;
    float delta_time;
    uint32_t particle_count;
    float gravity;
    float damping;
    float padding[2];
} EmitterData;

// Particle emitter
typedef struct ParticleEmitter
{
    SDL_GPUDevice *device;
    SDL_GPUComputePipeline *compute_pipeline;
    SDL_GPUBuffer *particle_buffer;
    SDL_GPUBuffer *emitter_buffer;
    
    Particle *particles;
    EmitterData emitter_data;
    
    uint32_t particle_count;
    bool active;
} ParticleEmitter;

bool particle_emitter_create(ParticleEmitter *emitter, SDL_GPUDevice *device, Vector2f position, uint32_t max_particles);
void particle_emitter_destroy(ParticleEmitter *emitter);
void particle_emitter_update(ParticleEmitter *emitter, float delta_time);
void particle_emitter_render(ParticleEmitter *emitter, BatchRenderer2D *batch_renderer);
void particle_emitter_set_position(ParticleEmitter *emitter, Vector2f position);
void particle_emitter_set_gravity(ParticleEmitter *emitter, float gravity);
void particle_emitter_set_damping(ParticleEmitter *emitter, float damping);

#endif
