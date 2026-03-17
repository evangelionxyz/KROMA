#version 450

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec4 color;
    float lifetime;
    float size;
    float padding[2];
};

layout(set = 0, binding = 0) readonly buffer ParticleBuffer
{
    Particle particles[];
};

layout(set = 1, binding = 0) buffer AliveParticleBuffer
{
    Particle alive_particles[];
};

layout(set = 1, binding = 1) buffer CounterBuffer
{
    uint alive_count;
};

layout(set = 2, binding = 0) uniform PushConstants
{
    uint particle_count;
} pc;

layout(local_size_x = 64) in;

void main()
{
    uint index = gl_GlobalInvocationID.x;
    
    if (index >= pc.particle_count)
        return;
    
    Particle p = particles[index];
    
    if (p.lifetime > 0.0)
    {
        uint write_index = atomicAdd(alive_count, 1u);
        alive_particles[write_index] = p;
    }
}
