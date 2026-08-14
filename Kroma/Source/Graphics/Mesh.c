// Copyright (c) 2026 Evangelion Manuhutu

#include "Mesh.h"
#include "GPUTexture.h"

#include <SDL3/SDL_log.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "cglm/cglm.h"

// ============================================================
// Helper: resolve the directory path from a file path
// ============================================================
static void get_directory(const char *filepath, char *dir_out, size_t dir_out_size)
{
    // Find last separator
    const char *last_sep = NULL;
    for (const char *p = filepath; *p; ++p)
    {
        if (*p == '/' || *p == '\\') last_sep = p;
    }

    if (last_sep)
    {
        size_t len = (size_t)(last_sep - filepath + 1);
        if (len >= dir_out_size) len = dir_out_size - 1;
        memcpy(dir_out, filepath, len);
        dir_out[len] = '\0';
    }
    else
    {
        dir_out[0] = '\0';
    }
}

// ============================================================
// Helper: load a GPU texture from a cgltf_image
// ============================================================
static GPUTexture load_texture_from_cgltf_image(SDL_GPUDevice *device, const cgltf_image *image, const char *base_dir)
{
    GPUTexture result = {0};

    if (!image)
        return result;

    // Try loading from buffer view (embedded data in .glb)
    if (image->buffer_view && image->buffer_view->buffer && image->buffer_view->buffer->data)
    {
        const unsigned char *data = (const unsigned char *)image->buffer_view->buffer->data;
        data += image->buffer_view->offset;
        result = gpu_texture_create_from_memory(device, data, (uint32_t)image->buffer_view->size);
        if (result.texture)
        {
            SDL_Log("Mesh: Loaded embedded texture '%s'", image->name ? image->name : "(unnamed)");
            return result;
        }
    }

    // Try loading from URI (external file)
    if (image->uri && strncmp(image->uri, "data:", 5) != 0)
    {
        char full_path[1024];
        SDL_snprintf(full_path, sizeof(full_path), "%s%s", base_dir, image->uri);
        result = gpu_texture_create_from_file(device, full_path);
        if (result.texture)
        {
            return result;
        }
    }

    return result;
}

// ============================================================
// Helper: read accessor data into a float array
// ============================================================
static float *read_accessor_floats(const cgltf_accessor *accessor, uint32_t components_per_element)
{
    if (!accessor) return NULL;

    const size_t count = accessor->count;
    float *data = (float *)malloc(sizeof(float) * count * components_per_element);
    if (!data) return NULL;

    for (size_t i = 0; i < count; ++i)
    {
        cgltf_accessor_read_float(accessor, i, &data[i * components_per_element], components_per_element);
    }

    return data;
}

// ============================================================
// Helper: read accessor indices
// ============================================================
static uint32_t *read_accessor_indices(const cgltf_accessor *accessor, uint32_t *out_count)
{
    if (!accessor)
    {
        *out_count = 0;
        return NULL;
    }

    const size_t count = accessor->count;
    uint32_t *data = (uint32_t *)malloc(sizeof(uint32_t) * count);
    if (!data)
    {
        *out_count = 0;
        return NULL;
    }

    for (size_t i = 0; i < count; ++i)
    {
        data[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
    }

    *out_count = (uint32_t)count;
    return data;
}

// ============================================================
// Helper: compute tangents when not provided by glTF
// Uses a simplified MikkTSpace-like approach
// ============================================================
static void compute_tangents(PBRVertex *vertices, uint32_t vertex_count, const uint32_t *indices, uint32_t index_count)
{
    // Accumulate tangents per-vertex
    vec3 *tan1 = (vec3 *)calloc(vertex_count, sizeof(vec3));
    vec3 *tan2 = (vec3 *)calloc(vertex_count, sizeof(vec3));
    if (!tan1 || !tan2)
    {
        free(tan1);
        free(tan2);
        // Fallback: set tangent to (1, 0, 0, 1)
        for (uint32_t i = 0; i < vertex_count; ++i)
        {
            glm_vec4_copy((vec4){1.0f, 0.0f, 0.0f, 1.0f}, vertices[i].tangent);
        }
        return;
    }

    for (uint32_t i = 0; i + 2 < index_count; i += 3)
    {
        const uint32_t i0 = indices[i + 0];
        const uint32_t i1 = indices[i + 1];
        const uint32_t i2 = indices[i + 2];

        const float *p0 = vertices[i0].position;
        const float *p1 = vertices[i1].position;
        const float *p2 = vertices[i2].position;

        const float *uv0 = vertices[i0].uv;
        const float *uv1 = vertices[i1].uv;
        const float *uv2 = vertices[i2].uv;

        vec3 edge1, edge2;
        glm_vec3_sub((float *)p1, (float *)p0, edge1);
        glm_vec3_sub((float *)p2, (float *)p0, edge2);

        float du1 = uv1[0] - uv0[0];
        float dv1 = uv1[1] - uv0[1];
        float du2 = uv2[0] - uv0[0];
        float dv2 = uv2[1] - uv0[1];

        float det = du1 * dv2 - du2 * dv1;
        if (fabsf(det) < 1e-8f) continue;

        float inv_det = 1.0f / det;

        vec3 sdir = {
            (dv2 * edge1[0] - dv1 * edge2[0]) * inv_det,
            (dv2 * edge1[1] - dv1 * edge2[1]) * inv_det,
            (dv2 * edge1[2] - dv1 * edge2[2]) * inv_det
        };

        vec3 tdir = {
            (du1 * edge2[0] - du2 * edge1[0]) * inv_det,
            (du1 * edge2[1] - du2 * edge1[1]) * inv_det,
            (du1 * edge2[2] - du2 * edge1[2]) * inv_det
        };

        glm_vec3_add(tan1[i0], sdir, tan1[i0]);
        glm_vec3_add(tan1[i1], sdir, tan1[i1]);
        glm_vec3_add(tan1[i2], sdir, tan1[i2]);

        glm_vec3_add(tan2[i0], tdir, tan2[i0]);
        glm_vec3_add(tan2[i1], tdir, tan2[i1]);
        glm_vec3_add(tan2[i2], tdir, tan2[i2]);
    }

    for (uint32_t i = 0; i < vertex_count; ++i)
    {
        vec3 n, t;
        glm_vec3_copy(vertices[i].normal, n);
        glm_vec3_copy(tan1[i], t);

        // Gram-Schmidt orthogonalize
        vec3 proj;
        glm_vec3_scale(n, glm_vec3_dot(n, t), proj);
        vec3 tangent;
        glm_vec3_sub(t, proj, tangent);
        glm_vec3_normalize(tangent);

        // Handedness
        vec3 cross;
        glm_vec3_cross(n, t, cross);
        float w = (glm_vec3_dot(cross, tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        vertices[i].tangent[0] = tangent[0];
        vertices[i].tangent[1] = tangent[1];
        vertices[i].tangent[2] = tangent[2];
        vertices[i].tangent[3] = w;
    }

    free(tan1);
    free(tan2);
}

// ============================================================
// Count total primitives across all meshes in all nodes
// ============================================================
static uint32_t count_total_primitives(const cgltf_data *data)
{
    uint32_t count = 0;
    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        count += (uint32_t)data->meshes[i].primitives_count;
    }
    return count;
}

// ============================================================
// Public: Load a static mesh from a glTF/glb file
// ============================================================
bool static_mesh_load_gltf(StaticMesh *mesh, SDL_GPUDevice *device, const char *filepath)
{
    if (!mesh || !device || !filepath)
    {
        return false;
    }

    memset(mesh, 0, sizeof(StaticMesh));
    mesh->device = device;
    glm_mat4_identity(mesh->transform);

    // Parse glTF
    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Failed to parse glTF file '%s' (error %d)", filepath, result);
        return false;
    }

    result = cgltf_load_buffers(&options, data, filepath);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Failed to load glTF buffers for '%s'", filepath);
        cgltf_free(data);
        return false;
    }

    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: glTF validation failed for '%s' (error %d)", filepath, result);
        cgltf_free(data);
        return false;
    }

    // Get base directory for resolving texture paths
    char base_dir[1024];
    get_directory(filepath, base_dir, sizeof(base_dir));

    // ========================================
    // Load Materials
    // ========================================
    mesh->material_count = (uint32_t)data->materials_count;
    if (mesh->material_count > 0)
    {
        mesh->materials = (PBRMaterial *)calloc(mesh->material_count, sizeof(PBRMaterial));
        if (!mesh->materials)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Failed to allocate materials");
            cgltf_free(data);
            return false;
        }

        for (uint32_t i = 0; i < mesh->material_count; ++i)
        {
            const cgltf_material *src = &data->materials[i];
            PBRMaterial *dst = &mesh->materials[i];

            // Start with defaults
            *dst = pbr_material_create_default(device);

            SDL_Log("Mesh: Loading material '%s'", src->name ? src->name : "(unnamed)");

            // PBR Metallic-Roughness
            if (src->has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness *pbr = &src->pbr_metallic_roughness;

                dst->base_color_factor = (Vector4f){
                    pbr->base_color_factor[0],
                    pbr->base_color_factor[1],
                    pbr->base_color_factor[2],
                    pbr->base_color_factor[3]
                };
                dst->metallic_factor = pbr->metallic_factor;
                dst->roughness_factor = pbr->roughness_factor;

                // Albedo texture
                if (pbr->base_color_texture.texture && pbr->base_color_texture.texture->image)
                {
                    GPUTexture tex = load_texture_from_cgltf_image(device, pbr->base_color_texture.texture->image, base_dir);
                    if (tex.texture)
                    {
                        gpu_texture_destroy(device, &dst->albedo_map);
                        dst->albedo_map = tex;
                    }
                }

                // Metallic-Roughness texture
                if (pbr->metallic_roughness_texture.texture && pbr->metallic_roughness_texture.texture->image)
                {
                    GPUTexture tex = load_texture_from_cgltf_image(device, pbr->metallic_roughness_texture.texture->image, base_dir);
                    if (tex.texture)
                    {
                        gpu_texture_destroy(device, &dst->metallic_roughness_map);
                        dst->metallic_roughness_map = tex;
                    }
                }
            }

            // Normal texture
            if (src->normal_texture.texture && src->normal_texture.texture->image)
            {
                GPUTexture tex = load_texture_from_cgltf_image(device, src->normal_texture.texture->image, base_dir);
                if (tex.texture)
                {
                    gpu_texture_destroy(device, &dst->normal_map);
                    dst->normal_map = tex;
                }
            }

            // Occlusion texture
            if (src->occlusion_texture.texture && src->occlusion_texture.texture->image)
            {
                GPUTexture tex = load_texture_from_cgltf_image(device, src->occlusion_texture.texture->image, base_dir);
                if (tex.texture)
                {
                    gpu_texture_destroy(device, &dst->occlusion_map);
                    dst->occlusion_map = tex;
                    dst->occlusion_strength = src->occlusion_texture.scale;
                }
            }

            // Emissive
            dst->emissive_factor = (Vector3f){
                src->emissive_factor[0],
                src->emissive_factor[1],
                src->emissive_factor[2]
            };
        }
    }

    // ========================================
    // Load Meshes (Primitives -> SubMeshes)
    // ========================================
    const uint32_t total_primitives = count_total_primitives(data);
    if (total_primitives == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: No primitives found in '%s'", filepath);
        cgltf_free(data);
        return false;
    }

    mesh->submeshes = (SubMesh *)calloc(total_primitives, sizeof(SubMesh));
    if (!mesh->submeshes)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Failed to allocate submeshes");
        cgltf_free(data);
        return false;
    }

    uint32_t submesh_index = 0;
    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi)
    {
        const cgltf_mesh *gltf_mesh = &data->meshes[mi];

        for (cgltf_size pi = 0; pi < gltf_mesh->primitives_count; ++pi)
        {
            const cgltf_primitive *prim = &gltf_mesh->primitives[pi];

            // Only handle triangle primitives
            if (prim->type != cgltf_primitive_type_triangles)
            {
                SDL_Log("Mesh: Skipping non-triangle primitive (type %d)", prim->type);
                continue;
            }

            // Find attribute accessors
            const cgltf_accessor *pos_accessor = NULL;
            const cgltf_accessor *normal_accessor = NULL;
            const cgltf_accessor *uv_accessor = NULL;
            const cgltf_accessor *tangent_accessor = NULL;

            for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai)
            {
                const cgltf_attribute *attr = &prim->attributes[ai];
                switch (attr->type)
                {
                    case cgltf_attribute_type_position:
                        pos_accessor = attr->data;
                        break;
                    case cgltf_attribute_type_normal:
                        normal_accessor = attr->data;
                        break;
                    case cgltf_attribute_type_texcoord:
                        if (attr->index == 0) uv_accessor = attr->data;
                        break;
                    case cgltf_attribute_type_tangent:
                        tangent_accessor = attr->data;
                        break;
                    default:
                        break;
                }
            }

            if (!pos_accessor)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Primitive has no POSITION attribute, skipping");
                continue;
            }

            const uint32_t vertex_count = (uint32_t)pos_accessor->count;

            // Read attribute data
            float *positions = read_accessor_floats(pos_accessor, 3);
            float *normals = normal_accessor ? read_accessor_floats(normal_accessor, 3) : NULL;
            float *uvs = uv_accessor ? read_accessor_floats(uv_accessor, 2) : NULL;
            float *tangents = tangent_accessor ? read_accessor_floats(tangent_accessor, 4) : NULL;

            // Build PBRVertex array
            PBRVertex *vertices = (PBRVertex *)calloc(vertex_count, sizeof(PBRVertex));
            if (!vertices)
            {
                free(positions);
                free(normals);
                free(uvs);
                free(tangents);
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Mesh: Failed to allocate vertex data");
                continue;
            }

            static float UNIT_CONVERSION = 1.0f / 100.0f;

            for (uint32_t vi = 0; vi < vertex_count; ++vi)
            {
                vertices[vi].position[0] = positions[vi * 3 + 0] * UNIT_CONVERSION;
                vertices[vi].position[1] = positions[vi * 3 + 1] * UNIT_CONVERSION;
                vertices[vi].position[2] = positions[vi * 3 + 2] * UNIT_CONVERSION;

                if (normals)
                {
                    vertices[vi].normal[0] = normals[vi * 3 + 0];
                    vertices[vi].normal[1] = normals[vi * 3 + 1];
                    vertices[vi].normal[2] = normals[vi * 3 + 2];
                }
                else
                {
                    vertices[vi].normal[0] = 0.0f;
                    vertices[vi].normal[1] = 1.0f;
                    vertices[vi].normal[2] = 0.0f;
                }

                if (uvs)
                {
                    vertices[vi].uv[0] = uvs[vi * 2 + 0];
                    vertices[vi].uv[1] = uvs[vi * 2 + 1];
                }

                if (tangents)
                {
                    vertices[vi].tangent[0] = tangents[vi * 4 + 0];
                    vertices[vi].tangent[1] = tangents[vi * 4 + 1];
                    vertices[vi].tangent[2] = tangents[vi * 4 + 2];
                    vertices[vi].tangent[3] = tangents[vi * 4 + 3];
                }
            }

            // Read indices
            uint32_t index_count = 0;
            uint32_t *indices = NULL;

            if (prim->indices)
            {
                indices = read_accessor_indices(prim->indices, &index_count);
            }
            else
            {
                // Generate sequential indices
                index_count = vertex_count;
                indices = (uint32_t *)malloc(sizeof(uint32_t) * index_count);
                if (indices)
                {
                    for (uint32_t vi = 0; vi < vertex_count; ++vi)
                    {
                        indices[vi] = vi;
                    }
                }
            }

            // Compute tangents if not provided
            if (!tangents && indices)
            {
                compute_tangents(vertices, vertex_count, indices, index_count);
            }

            // Create GPU buffers
            SubMesh *sub = &mesh->submeshes[submesh_index];
            sub->vertex_buffer = vertex_buffer_create(device, vertices, (uint32_t)(vertex_count * sizeof(PBRVertex)), vertex_count);
            sub->index_buffer = index_buffer_create(device, indices, (uint32_t)(index_count * sizeof(uint32_t)), index_count);

            // Resolve material index
            if (prim->material)
            {
                sub->material_index = (uint32_t)(prim->material - data->materials);
            }
            else
            {
                sub->material_index = 0;
            }

            SDL_Log("Mesh: Loaded submesh %u: %u vertices, %u indices, material %u",
                submesh_index, vertex_count, index_count, sub->material_index);

            // Cleanup CPU data
            free(positions);
            free(normals);
            free(uvs);
            free(tangents);
            free(vertices);
            free(indices);

            submesh_index++;
        }
    }

    mesh->submesh_count = submesh_index;

    SDL_Log("Mesh: Loaded '%s' — %u submeshes, %u materials", filepath, mesh->submesh_count, mesh->material_count);

    cgltf_free(data);
    return true;
}

// ============================================================
// Public: Destroy a static mesh
// ============================================================
void static_mesh_destroy(StaticMesh *mesh)
{
    if (!mesh || !mesh->device)
        return;

    // Destroy submeshes
    if (mesh->submeshes)
    {
        for (uint32_t i = 0; i < mesh->submesh_count; ++i)
        {
            vertex_buffer_destroy(mesh->device, &mesh->submeshes[i].vertex_buffer);
            index_buffer_destroy(mesh->device, &mesh->submeshes[i].index_buffer);
        }
        free(mesh->submeshes);
        mesh->submeshes = NULL;
    }

    // Destroy materials
    if (mesh->materials)
    {
        for (uint32_t i = 0; i < mesh->material_count; ++i)
        {
            pbr_material_destroy(mesh->device, &mesh->materials[i]);
        }
        free(mesh->materials);
        mesh->materials = NULL;
    }

    mesh->submesh_count = 0;
    mesh->material_count = 0;
    mesh->device = NULL;
}

// ============================================================
// Public: Draw all sub-meshes
// ============================================================
void static_mesh_draw(const StaticMesh *mesh, SDL_GPURenderPass *render_pass)
{
    if (!mesh || !render_pass)
        return;

    for (uint32_t i = 0; i < mesh->submesh_count; ++i)
    {
        const SubMesh *sub = &mesh->submeshes[i];

        if (!sub->vertex_buffer.buffer || !sub->index_buffer.buffer)
            continue;

        // Bind material textures
        if (mesh->materials && sub->material_index < mesh->material_count)
        {
            pbr_material_bind(&mesh->materials[sub->material_index], render_pass);
        }

        // Bind vertex buffer
        SDL_GPUBufferBinding vertex_binding = {0};
        vertex_binding.buffer = sub->vertex_buffer.buffer;
        vertex_binding.offset = 0;
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

        // Bind index buffer
        SDL_GPUBufferBinding index_binding = {0};
        index_binding.buffer = sub->index_buffer.buffer;
        index_binding.offset = 0;
        SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // Draw
        SDL_DrawGPUIndexedPrimitives(render_pass, sub->index_buffer.num_indices, 1, 0, 0, 0);
    }
}
