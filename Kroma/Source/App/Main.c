// Copyright (c) 2026 Evangelion Manuhutu

#include "SDL3/SDL_gpu.h"
#include "Graphics/DeviceManager.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/Renderer.h"
#include "Graphics/GPUBuffers.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Terrain.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"

#include "Core/Base.h"
#include "Core/Camera.h"
#include "Core/Math.h"

#include "cglm/cglm.h"

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    // Initialize the shared GPU Device Manager
    DeviceManager device_manager = {0};
    if (!device_manager_init(&device_manager, SDL_GPU_SHADERFORMAT_SPIRV, false))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GPU Device Manager");
        SDL_Quit();
        return -1;
    }

    SDL_GPUDevice *device = device_manager_get_device(&device_manager);

    // Create and claim main window
    Window window = {0};
    if (!window_create(&window, "KROMA", 1280, 640, SDL_WINDOW_RESIZABLE) ||
        !device_manager_claim_window(&device_manager, &window))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create or claim main window");
        device_manager_shutdown(&device_manager);
        SDL_Quit();
        return -1;
    }

    // Create scene render target and depth target
    SDL_GPUTexture *scene_rt_texture = NULL;
    SDL_GPUTexture *scene_depth_texture = NULL;
    SDL_GPUSampler *scene_rt_sampler = NULL;

    SDL_GPUTextureCreateInfo scene_rt_texture_info = {0};
    scene_rt_texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    scene_rt_texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    scene_rt_texture_info.width = window.width;
    scene_rt_texture_info.height = window.height;
    scene_rt_texture_info.layer_count_or_depth = 1;
    scene_rt_texture_info.num_levels = 1;
    scene_rt_texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    scene_rt_texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    scene_rt_texture = SDL_CreateGPUTexture(device, &scene_rt_texture_info);

    // Determine supported depth format (D32_FLOAT -> D16_UNORM -> D24_S8)
    SDL_GPUTextureFormat depth_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    if (!SDL_GPUTextureSupportsFormat(device, depth_format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    {
        depth_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        if (!SDL_GPUTextureSupportsFormat(device, depth_format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
        {
            depth_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
        }
    }
    SDL_Log("Using depth format: %d", depth_format);

    SDL_GPUTextureCreateInfo scene_depth_texture_info = {0};
    scene_depth_texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    scene_depth_texture_info.format = depth_format;
    scene_depth_texture_info.width = window.width;
    scene_depth_texture_info.height = window.height;
    scene_depth_texture_info.layer_count_or_depth = 1;
    scene_depth_texture_info.num_levels = 1;
    scene_depth_texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    scene_depth_texture_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    scene_depth_texture = SDL_CreateGPUTexture(device, &scene_depth_texture_info);
    if (!scene_depth_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create scene depth texture: %s", SDL_GetError());
    }

    // Create sampler for the scene texture
    SDL_GPUSamplerCreateInfo sampler_info = {0};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    scene_rt_sampler = SDL_CreateGPUSampler(device, &sampler_info);

    // Initialize batch renderer (supports up to 1 million quads)
    BatchRenderer2D batch_renderer = {0};
    if (!batch_renderer_2d_init(&batch_renderer, device))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize batch renderer");
        device_manager_release_window(&device_manager, &window);
        window_destroy(&window);
        device_manager_shutdown(&device_manager);
        SDL_Quit();
        return -1;
    }

    SDL_GPUGraphicsPipeline *composite_pso = NULL;
    SDL_GPUGraphicsPipeline *two_dim_pso = NULL;
    SDL_GPUGraphicsPipeline *three_dim_pso = NULL;
    SDL_GPUGraphicsPipeline *pbr_pso = NULL;
    SDL_GPUFillMode pso_fill_mode = SDL_GPU_FILLMODE_FILL;
    bool recreate_psos = true; // Create when initialization

    UniformBuffer view_projection_buffer = uniform_buffer_create(device, sizeof(mat4));
    UniformBuffer model_matrix_buffer = uniform_buffer_create(device, sizeof(mat4));
    UniformBuffer material_params_buffer = uniform_buffer_create(device, sizeof(MaterialGPUData));

    Camera camera = {0};
    camera_init(&camera, (Vector3f){0.0f, 0.0f, 0.0f}, 0.0f, 0.0f,
        window.width, window.height, glm_rad(90.0f));

    mat4 view_projection;
    camera_update_matrices(&camera, view_projection);
    uniform_buffer_update(device, &view_projection_buffer, view_projection, sizeof(mat4));

    bool mouse_look_enabled = false;

    // Create particle emitter
    // ParticleEmitter particle_emitter = {0};
    // if (!particle_emitter_create(&particle_emitter, device, (Vector2f){0.0f, 0.0f}, 200))
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create particle emitter");
    //     batch_renderer_2d_destroy(&batch_renderer);
    //     device_manager_release_window(&device_manager, &window);
    //     window_destroy(&window);
    //     device_manager_shutdown(&device_manager);
    //     SDL_Quit();
    //     return -1;
    // }

    // Terrain terrain = {0};
    // if (!terrain_create(&terrain, device, 128, 128, 0.25f, 2.5f, 1337))
    // {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create terrain");
    //     particle_emitter_destroy(&particle_emitter);
    //     batch_renderer_2d_destroy(&batch_renderer);
    //     uniform_buffer_destroy(device, &view_projection_buffer);
    //     SDL_ReleaseGPUSampler(device, scene_rt_sampler);
    //     SDL_ReleaseGPUTexture(device, scene_rt_texture);
    //     device_manager_release_window(&device_manager, &window);
    //     window_destroy(&window);
    //     device_manager_shutdown(&device_manager);
    //     SDL_Quit();
    //     return -1;
    // }

    // Load glTF mesh
    StaticMesh pbr_mesh = {0};
    const char *test_model_filepath = "Resources/Models/Fox.glb";
    bool mesh_loaded = static_mesh_load_gltf(&pbr_mesh, device, test_model_filepath);
    if (!mesh_loaded)
    {
        SDL_Log("No glTF model found at %s - PBR mesh rendering disabled", test_model_filepath);
    }

    // Set particle emitter properties
    // particle_emitter_set_gravity(&particle_emitter, -9.8f);
    // particle_emitter_set_damping(&particle_emitter, 0.1f);

    uint64_t last_time = SDL_GetPerformanceCounter();
    const uint64_t frequency = SDL_GetPerformanceFrequency();

    SDL_Event event;
    bool running = true;
    while (running && window_is_open(&window))
    {
        // Calculate delta time
        const uint64_t current_time = SDL_GetPerformanceCounter();
        const float delta_time = (float)(current_time - last_time) / (float)frequency;
        last_time = current_time;

        // Update particle emitter
        // particle_emitter_update(&particle_emitter, delta_time);

        while (SDL_PollEvent(&event))
        {
            window_handle_event(&window, &event);

            switch (event.type)
            {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                {
                    if (event.window.windowID == window.id)
                    {
                        // Recreate scene texture with new dimensions
                        if (scene_rt_texture)
                        {
                            SDL_ReleaseGPUTexture(device, scene_rt_texture);
                        }
                        scene_rt_texture_info.width = window.width;
                        scene_rt_texture_info.height = window.height;
                        scene_rt_texture = SDL_CreateGPUTexture(device, &scene_rt_texture_info);

                        if (scene_depth_texture)
                        {
                            SDL_ReleaseGPUTexture(device, scene_depth_texture);
                        }
                        scene_depth_texture_info.width = window.width;
                        scene_depth_texture_info.height = window.height;
                        scene_depth_texture = SDL_CreateGPUTexture(device, &scene_depth_texture_info);

                        // Update camera immediately
                        camera_set_viewport(&camera, window.width, window.height);
                        camera_update_matrices(&camera, view_projection);
                        uniform_buffer_update(device, &view_projection_buffer, view_projection, sizeof(mat4));
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    if (event.window.windowID == window.id)
                    {
                        running = false;
                    }
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        running = false;
                    }

                    // Changing Fill Mode
                    if (event.key.key == SDLK_1 && pso_fill_mode != SDL_GPU_FILLMODE_FILL)
                    {
                        recreate_psos = true;
                        pso_fill_mode = SDL_GPU_FILLMODE_FILL;
                    }
                    else if (event.key.key == SDLK_2 && pso_fill_mode != SDL_GPU_FILLMODE_LINE)
                    {
                        recreate_psos = true;
                        pso_fill_mode = SDL_GPU_FILLMODE_LINE;
                    }

                    
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    if (event.button.button == SDL_BUTTON_RIGHT)
                    {
                        mouse_look_enabled = true;
                        SDL_SetWindowRelativeMouseMode(window.handle, true);
                    }

                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        const Vector3f world_pos = get_world_coordinate(&camera, event.button.x, event.button.y);
                        // particle_emitter_set_position(&particle_emitter, (Vector2f){world_pos.x, world_pos.y});
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    if (event.button.button == SDL_BUTTON_RIGHT)
                    {
                        mouse_look_enabled = false;
                        SDL_SetWindowRelativeMouseMode(window.handle, false);
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION:
                {
                    if (mouse_look_enabled)
                    {
                        camera_process_mouse(&camera, event.motion.xrel, event.motion.yrel);
                    }
                    break;
                }
                case SDL_EVENT_QUIT:
                {
                    running = false;
                    break;
                }
                default:
                    break;
            }
        }

        // Process keyboard input for camera
        const bool *keyboard_state = SDL_GetKeyboardState(NULL);
        camera_process_keyboard(&camera, keyboard_state, delta_time);
        camera_update_matrices(&camera, view_projection);
        uniform_buffer_update(device, &view_projection_buffer, view_projection, sizeof(mat4));

        if (recreate_psos)
        {
            // Destroy PSOs
            graphics_pipeline_destroy(device, three_dim_pso);
            graphics_pipeline_destroy(device, two_dim_pso);
            graphics_pipeline_destroy(device, pbr_pso);
            three_dim_pso = NULL;
            two_dim_pso = NULL;
            pbr_pso = NULL;
            
            // Create 3D pipeline
            Shader vertex_shader_3d = shader_create(device, SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/3d.vert.glsl", "main", KR_FALSE);
            Shader fragment_shader_3d = shader_create(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/3d.frag.glsl", "main", KR_FALSE);

            const uint32_t vertex_stride_3d = shader_calculate_vertex_stride(&vertex_shader_3d.reflection_info);

            SDL_GPUVertexBufferDescription vertex_buffer_desc_3d = {0};
            vertex_buffer_desc_3d.slot = 0;
            vertex_buffer_desc_3d.pitch = vertex_stride_3d;
            vertex_buffer_desc_3d.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertex_buffer_desc_3d.instance_step_rate = 0;

            GraphicsPipelineDescription three_dim_pso_desc = {0};
            three_dim_pso_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            three_dim_pso_desc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            three_dim_pso_desc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            three_dim_pso_desc.fill_mode = pso_fill_mode;
            three_dim_pso_desc.cull_mode = SDL_GPU_CULLMODE_NONE;
            three_dim_pso_desc.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
            three_dim_pso_desc.vertex_shader = &vertex_shader_3d;
            three_dim_pso_desc.fragment_shader = &fragment_shader_3d;
            three_dim_pso_desc.enable_depth_test = true;
            three_dim_pso_desc.enable_depth_write = true;
            three_dim_pso_desc.enable_blend = false;
            three_dim_pso_desc.depth_stencil_format = depth_format;
            three_dim_pso_desc.vertex_buffer_descriptions = &vertex_buffer_desc_3d;
            three_dim_pso_desc.num_vertex_buffers = 1;

            three_dim_pso = graphics_pipeline_create(device, &three_dim_pso_desc);
            if (!three_dim_pso)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create 3D pipeline");
            }

            shader_release(device, &vertex_shader_3d);
            shader_release(device, &fragment_shader_3d);

            // Create 2D pipeline
            Shader vertex_shader_2d = shader_create(device, SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/2d.vert.glsl", "main", KR_FALSE);
            Shader fragment_shader_2d = shader_create(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/2d.frag.glsl", "main", KR_FALSE);

            // Log the reflected vertex attributes
            SDL_Log("Vertex shader has %u attributes:", vertex_shader_2d.reflection_info.vertex_attribute_count);
            for (uint32_t i = 0; i < vertex_shader_2d.reflection_info.vertex_attribute_count; ++i)
            {
                SDL_GPUVertexAttribute *attr = &vertex_shader_2d.reflection_info.vertex_attributes[i];
                SDL_Log("  Attribute %u: location=%u, buffer_slot=%u, format=%d, offset=%u", i, attr->location, attr->buffer_slot, attr->format, attr->offset);
            }

            // Calculate vertex stride from reflected attributes
            const uint32_t vertex_stride = shader_calculate_vertex_stride(&vertex_shader_2d.reflection_info);

            SDL_GPUVertexBufferDescription vertex_buffer_desc = {0};
            vertex_buffer_desc.slot = 0;
            vertex_buffer_desc.pitch = vertex_stride;
            vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertex_buffer_desc.instance_step_rate = 0;

            GraphicsPipelineDescription two_dim_pso_desc = {0};
            two_dim_pso_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            two_dim_pso_desc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            two_dim_pso_desc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            two_dim_pso_desc.fill_mode = pso_fill_mode;
            two_dim_pso_desc.cull_mode = SDL_GPU_CULLMODE_NONE;
            two_dim_pso_desc.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
            two_dim_pso_desc.vertex_shader = &vertex_shader_2d;
            two_dim_pso_desc.fragment_shader = &fragment_shader_2d;
            two_dim_pso_desc.enable_depth_test = false;
            two_dim_pso_desc.enable_depth_write = false;
            two_dim_pso_desc.enable_blend = false;
            two_dim_pso_desc.depth_stencil_format = depth_format;
            two_dim_pso_desc.vertex_buffer_descriptions = &vertex_buffer_desc;
            two_dim_pso_desc.num_vertex_buffers = 1;

            two_dim_pso = graphics_pipeline_create(device, &two_dim_pso_desc);
            if (!two_dim_pso)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create 2D pipeline");
            }
            shader_release(device, &vertex_shader_2d);
            shader_release(device, &fragment_shader_2d);

            // Create composite pipeline
            if (!composite_pso)
            {
                Shader vertex_shader = shader_create(device, SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/composite.vert.glsl", "main", KR_FALSE);
                Shader fragment_shader = shader_create(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/composite.frag.glsl", "main", KR_FALSE);

                if (vertex_shader.handle && fragment_shader.handle)
                {
                    GraphicsPipelineDescription comp_pso_desc = {0};
                    comp_pso_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
                    comp_pso_desc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
                    comp_pso_desc.format = window.swapchain_format;
                    comp_pso_desc.fill_mode = SDL_GPU_FILLMODE_FILL;
                    comp_pso_desc.cull_mode = SDL_GPU_CULLMODE_NONE;
                    comp_pso_desc.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
                    comp_pso_desc.vertex_shader = &vertex_shader;
                    comp_pso_desc.fragment_shader = &fragment_shader;
                    comp_pso_desc.enable_depth_test = false;
                    comp_pso_desc.enable_depth_write = false;
                    comp_pso_desc.enable_blend = false;
                    comp_pso_desc.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
                    comp_pso_desc.vertex_buffer_descriptions = NULL;
                    comp_pso_desc.num_vertex_buffers = 0;

                    composite_pso = graphics_pipeline_create(device, &comp_pso_desc);
                    if (!composite_pso)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create composite pipeline: %s", SDL_GetError());
                    }
                }

                shader_release(device, &vertex_shader);
                shader_release(device, &fragment_shader);
            }

            // Create PBR pipeline
            if (mesh_loaded)
            {
                Shader pbr_vert = shader_create(device, SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/pbr.vert.glsl", "main", KR_FALSE);
                Shader pbr_frag = shader_create(device, SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/pbr.frag.glsl", "main", KR_FALSE);

                if (pbr_vert.handle && pbr_frag.handle)
                {
                    const uint32_t pbr_vertex_stride = shader_calculate_vertex_stride(&pbr_vert.reflection_info);

                    SDL_GPUVertexBufferDescription pbr_vb_desc = {0};
                    pbr_vb_desc.slot = 0;
                    pbr_vb_desc.pitch = pbr_vertex_stride;
                    pbr_vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
                    pbr_vb_desc.instance_step_rate = 0;

                    GraphicsPipelineDescription pbr_pso_desc = {0};
                    pbr_pso_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
                    pbr_pso_desc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
                    pbr_pso_desc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
                    pbr_pso_desc.fill_mode = pso_fill_mode;
                    pbr_pso_desc.cull_mode = SDL_GPU_CULLMODE_FRONT;
                    pbr_pso_desc.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
                    pbr_pso_desc.vertex_shader = &pbr_vert;
                    pbr_pso_desc.fragment_shader = &pbr_frag;
                    pbr_pso_desc.enable_depth_test = true;
                    pbr_pso_desc.enable_depth_write = true;
                    pbr_pso_desc.enable_blend = false;
                    pbr_pso_desc.depth_stencil_format = depth_format;
                    pbr_pso_desc.vertex_buffer_descriptions = &pbr_vb_desc;
                    pbr_pso_desc.num_vertex_buffers = 1;

                    pbr_pso = graphics_pipeline_create(device, &pbr_pso_desc);
                    if (!pbr_pso)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create PBR pipeline");
                    }
                }

                shader_release(device, &pbr_vert);
                shader_release(device, &pbr_frag);
            }

            recreate_psos = false;
        }

        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        Swapchain swapchain = {0};
        if (!swapchain_acquire(cmd, &window, &swapchain))
        {
            SDL_CancelGPUCommandBuffer(cmd);
            break;
        }

        // First pass: Render 2D quad to scene render target
        {
            SDL_GPUColorTargetInfo scene_target_info = {0};
            scene_target_info.texture = scene_rt_texture;
            scene_target_info.clear_color.r = 0.025f;
            scene_target_info.clear_color.g = 0.025f;
            scene_target_info.clear_color.b = 0.025f;
            scene_target_info.clear_color.a = 1.0f;
            scene_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            scene_target_info.store_op = SDL_GPU_STOREOP_STORE;
            scene_target_info.cycle = true;

            SDL_GPUDepthStencilTargetInfo scene_depth_target_info = {0};
            scene_depth_target_info.texture = scene_depth_texture;
            scene_depth_target_info.clear_depth = 1.0f;
            scene_depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            scene_depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
            scene_depth_target_info.cycle = true;

            SDL_GPURenderPass *scene_pass = SDL_BeginGPURenderPass(cmd, &scene_target_info, 1, &scene_depth_target_info);
            if (scene_pass)
            {
                SDL_GPUBuffer *uniform_buffers[] = { view_projection_buffer.buffer };

                if (three_dim_pso)
                {
                    // SDL_BindGPUGraphicsPipeline(scene_pass, three_dim_pso);
                    // SDL_BindGPUVertexStorageBuffers(scene_pass, 0, uniform_buffers, ARRAY_SIZE(uniform_buffers));
                    // terrain_draw(&terrain, scene_pass);
                }

                // PBR mesh rendering
                if (pbr_pso && mesh_loaded)
                {
                    SDL_BindGPUGraphicsPipeline(scene_pass, pbr_pso);

                    // Update model matrix
                    uniform_buffer_update(device, &model_matrix_buffer, pbr_mesh.transform, sizeof(mat4));

                    // Update material params with camera + light info
                    Vector3f cam_pos = {camera.position[0], camera.position[1], camera.position[2]};
                    Vector3f light_pos = {5.0f, 10.0f, 5.0f};
                    Vector3f light_col = {1.0f, 1.0f, 1.0f};
                    float light_intensity = 300.0f;

                    // Use the first material for GPU params (per-submesh textures are bound in static_mesh_draw)
                    PBRMaterial default_mat_params = {0};
                    default_mat_params.base_color_factor = (Vector4f){1.0f, 1.0f, 1.0f, 1.0f};
                    default_mat_params.metallic_factor = 1.0f;
                    default_mat_params.roughness_factor = 1.0f;
                    default_mat_params.occlusion_strength = 1.0f;
                    default_mat_params.emissive_factor = (Vector3f){0.0f, 0.0f, 0.0f};

                    const PBRMaterial *mat_for_params = (pbr_mesh.material_count > 0) ? &pbr_mesh.materials[0] : &default_mat_params;
                    MaterialGPUData gpu_data = pbr_material_get_gpu_data(mat_for_params, cam_pos, light_pos, light_intensity, light_col);
                    uniform_buffer_update(device, &material_params_buffer, &gpu_data, sizeof(MaterialGPUData));

                    // Bind storage buffers: slot 0 = view-proj, slot 1 = model, slot 2 = material params
                    SDL_GPUBuffer *pbr_storage_buffers[] = {
                        view_projection_buffer.buffer,
                        model_matrix_buffer.buffer,
                        material_params_buffer.buffer
                    };
                    SDL_BindGPUVertexStorageBuffers(scene_pass, 0, pbr_storage_buffers, 2);
                    SDL_BindGPUFragmentStorageBuffers(scene_pass, 0, &material_params_buffer.buffer, 1);

                    static_mesh_draw(&pbr_mesh, scene_pass);
                }

                if (two_dim_pso)
                {
                    SDL_BindGPUGraphicsPipeline(scene_pass, two_dim_pso);
                    SDL_BindGPUVertexStorageBuffers(scene_pass, 0, uniform_buffers, ARRAY_SIZE(uniform_buffers));

                    // Build batch of quads
                    batch_renderer_2d_begin(&batch_renderer);

                    // Render particles
                    // particle_emitter_render(&particle_emitter, &batch_renderer);

                    batch_renderer_2d_end(&batch_renderer);

                    // Draw all batched quads
                    batch_renderer_2d_draw(&batch_renderer, scene_pass);
                }

                SDL_EndGPURenderPass(scene_pass);
            }
        }

        // Second pass: Render composite to swapchain
        if (swapchain.texture)
        {
            SDL_GPUColorTargetInfo color_target_info = {0};
            color_target_info.texture = swapchain.texture;
            color_target_info.clear_color.r = 0.1f;
            color_target_info.clear_color.g = 0.1f;
            color_target_info.clear_color.b = 0.1f;
            color_target_info.clear_color.a = 1.0f;
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            color_target_info.cycle = false;

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd, &color_target_info, 1, NULL);
            if (render_pass && composite_pso)
            {
                SDL_BindGPUGraphicsPipeline(render_pass, composite_pso);

                SDL_GPUTextureSamplerBinding texture_binding = {0};
                texture_binding.texture = scene_rt_texture;
                texture_binding.sampler = scene_rt_sampler;

                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
                SDL_EndGPURenderPass(render_pass);
            }
        }

        if (!SDL_SubmitGPUCommandBuffer(cmd))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to submit GPU command buffer: %s", SDL_GetError());
            break;
        }
    }

    SDL_WaitForGPUIdle(device);

    graphics_pipeline_destroy(device, composite_pso);
    graphics_pipeline_destroy(device, two_dim_pso);
    graphics_pipeline_destroy(device, three_dim_pso);
    graphics_pipeline_destroy(device, pbr_pso);

    if (mesh_loaded) static_mesh_destroy(&pbr_mesh);
    // terrain_destroy(&terrain);
    // particle_emitter_destroy(&particle_emitter);
    batch_renderer_2d_destroy(&batch_renderer);
    uniform_buffer_destroy(device, &material_params_buffer);
    uniform_buffer_destroy(device, &model_matrix_buffer);
    uniform_buffer_destroy(device, &view_projection_buffer);

    SDL_ReleaseGPUSampler(device, scene_rt_sampler);
    if (scene_depth_texture)
    {
        SDL_ReleaseGPUTexture(device, scene_depth_texture);
    }
    SDL_ReleaseGPUTexture(device, scene_rt_texture);

    device_manager_release_window(&device_manager, &window);
    window_destroy(&window);
    device_manager_shutdown(&device_manager);

    SDL_Quit();

    return 0;
}
