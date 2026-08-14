#version 450

layout(location = 0) in vec3 frag_world_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in vec3 frag_tangent;
layout(location = 4) in vec3 frag_bitangent;

layout(location = 0) out vec4 out_color;

// Material textures (set 2 = fragment resources, samplers first)
layout(set = 2, binding = 0) uniform sampler2D albedo_map;
layout(set = 2, binding = 1) uniform sampler2D normal_map;
layout(set = 2, binding = 2) uniform sampler2D metallic_roughness_map;
layout(set = 2, binding = 3) uniform sampler2D occlusion_map;

// Material params & lighting (set 2 = fragment resources, storage buffers after samplers)
layout(set = 2, binding = 4) buffer MaterialParams
{
    vec4 base_color_factor;
    vec4 emissive_factor;       // xyz = emissive, w = occlusion_strength
    vec4 metallic_roughness;    // x = metallic, y = roughness
    vec4 camera_pos;            // xyz = camera world position
    vec4 light_position;        // xyz = position, w = intensity
    vec4 light_color;           // xyz = color
};

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz normal distribution
float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

// Schlick-GGX geometry function
float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometry_schlick_ggx(NdotV, roughness);
    float ggx2 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnel_schlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

void main()
{
    // Sample textures
    vec4 albedo = texture(albedo_map, frag_uv) * base_color_factor;
    vec3 normal_sample = texture(normal_map, frag_uv).rgb * 2.0 - 1.0;
    vec2 mr = texture(metallic_roughness_map, frag_uv).bg;  // B=metallic, G=roughness (glTF)
    float ao = texture(occlusion_map, frag_uv).r;

    float metallic = mr.x * metallic_roughness.x;
    float roughness = mr.y * metallic_roughness.y;
    roughness = clamp(roughness, 0.04, 1.0);
    float occlusion_str = emissive_factor.w;

    // Construct TBN matrix and apply normal mapping
    mat3 TBN = mat3(normalize(frag_tangent), normalize(frag_bitangent), normalize(frag_normal));
    vec3 N = normalize(TBN * normal_sample);
    vec3 V = normalize(camera_pos.xyz - frag_world_pos);

    // F0: reflectance at normal incidence
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    // === Point Light ===
    vec3 light_dir = light_position.xyz - frag_world_pos;
    float light_dist = length(light_dir);
    vec3 L = normalize(light_dir);
    vec3 H = normalize(V + L);

    // Point light attenuation (inverse square)
    float attenuation = 1.0 / (light_dist * light_dist + 0.0001);
    float light_intensity = light_position.w;
    vec3 radiance = light_color.xyz * light_intensity * attenuation;

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo.rgb / PI + specular) * radiance * NdotL;

    // Ambient + AO
    vec3 ambient = vec3(0.03) * albedo.rgb * mix(1.0, ao, occlusion_str);

    // Emissive
    vec3 emissive = emissive_factor.xyz;

    vec3 color = ambient + Lo + emissive;

    // Tone mapping (Reinhard) + gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, albedo.a);
}
