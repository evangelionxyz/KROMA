#include "Swapchain.h"
#include "Window.h"
#include "GraphicsPipeline.h"
#include "Shader.h"
#include "Buffers.h"
#include "Renderer.h"

#include "Math.h"

#include "cglm/cglm.h"

typedef struct Camera
{
    vec3 position;
    mat4 view;
    mat4 projection;
    Vector2i size;
} Camera;

void camera_update_matrices(Camera *camera, int width, int height, mat4 view_projection)
{
    camera->size.x = width;
    camera->size.y = height;

    const float aspect = (float)width / (float)height;
    glm_perspective(glm_rad(45.0f), aspect, 0.1f, 500.0f, camera->projection);
    glm_lookat(camera->position, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, camera->view);

    glm_mat4_mul(camera->projection, camera->view, view_projection);
}

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    int SCREEN_WIDTH = 720;
    int SCREEN_HEIGHT = 540;
    
    Window window = {0};
    create_window(&window, "KROMA", 720, 540);
    create_gpu_device(&window);

    // Create scene render target
    SDL_GPUTextureCreateInfo scene_texture_info = {0};
    scene_texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    scene_texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    scene_texture_info.width = SCREEN_WIDTH;
    scene_texture_info.height = SCREEN_HEIGHT;
    scene_texture_info.layer_count_or_depth = 1;
    scene_texture_info.num_levels = 1;
    scene_texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    scene_texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture *scene_texture = SDL_CreateGPUTexture(window.device, &scene_texture_info);

    // Create sampler for the scene texture
    SDL_GPUSamplerCreateInfo sampler_info = {0};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(window.device, &sampler_info);

    // Initialize batch renderer (supports up to 1 million quads)
    BatchRenderer2D batch_renderer = {0};
    if (!batch_renderer_2d_init(&batch_renderer, window.device))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize batch renderer");
        SDL_Quit();
        return -1;
    }

    SDL_GPUGraphicsPipeline *composite_pipeline = NULL;
    SDL_GPUGraphicsPipeline *two_dimension_pipeline = NULL;

    SDL_Event event;
    bool running = true;

    // Create uniform buffer for view-projection matrix
    UniformBuffer view_projection_buffer = uniform_buffer_create(window.device, sizeof(mat4));

    Camera camera;
    glm_vec3_zero(camera.position);
    camera.position[2] = -8.0f;

    mat4 view_projection;
    camera_update_matrices(&camera, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, view_projection);
    uniform_buffer_update(window.device, &view_projection_buffer, view_projection, sizeof(mat4));

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                {
                    SCREEN_WIDTH = event.window.data1;
                    SCREEN_HEIGHT = event.window.data2;
                    
                    // Recreate scene texture with new dimensions
                    if (scene_texture)
                    {
                        SDL_ReleaseGPUTexture(window.device, scene_texture);
                    }
                    scene_texture_info.width = SCREEN_WIDTH;
                    scene_texture_info.height = SCREEN_HEIGHT;
                    scene_texture = SDL_CreateGPUTexture(window.device, &scene_texture_info);
                    
                    // Update camera immediately
                    camera_update_matrices(&camera, SCREEN_WIDTH, SCREEN_HEIGHT, view_projection);
                    uniform_buffer_update(window.device, &view_projection_buffer, view_projection, sizeof(mat4));
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        running = false;
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION:
                {
                    break;
                }
            }
        }

        if (!composite_pipeline)
        {
            // Create 2D pipeline
            Shader vertex_shader_2d = shader_create(window.device,
                SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/2d.vert.spv",
                "main");

            Shader fragment_shader_2d = shader_create(window.device,
                SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/2d.frag.spv",
                "main");

            // Log the reflected vertex attributes
            SDL_Log("Vertex shader has %u attributes:", vertex_shader_2d.reflection_info.vertex_attribute_count);
            for (uint32_t i = 0; i < vertex_shader_2d.reflection_info.vertex_attribute_count; ++i)
            {
                SDL_GPUVertexAttribute *attr = &vertex_shader_2d.reflection_info.vertex_attributes[i];
                SDL_Log("  Attribute %u: location=%u, buffer_slot=%u, format=%d, offset=%u",
                        i, attr->location, attr->buffer_slot, attr->format, attr->offset);
            }

            // Calculate vertex stride from reflected attributes
            uint32_t vertex_stride = 0;
            for (uint32_t i = 0; i < vertex_shader_2d.reflection_info.vertex_attribute_count; ++i)
            {
                SDL_GPUVertexAttribute *attr = &vertex_shader_2d.reflection_info.vertex_attributes[i];
                uint32_t attr_size = 0;
                
                // Calculate size based on format
                switch (attr->format)
                {
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT:
                        attr_size = 4;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT2:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT2:
                        attr_size = 8;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT3:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT3:
                        attr_size = 12;
                        break;
                    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
                    case SDL_GPU_VERTEXELEMENTFORMAT_INT4:
                    case SDL_GPU_VERTEXELEMENTFORMAT_UINT4:
                        attr_size = 16;
                        break;
                    default:
                        break;
                }
                vertex_stride += attr_size;
            }

            SDL_GPUVertexBufferDescription vertex_buffer_desc = {0};
            vertex_buffer_desc.slot = 0;
            vertex_buffer_desc.pitch = vertex_stride;
            vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertex_buffer_desc.instance_step_rate = 0;

            GraphicsPipelineDescription desc_2d = {0};
            desc_2d.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            desc_2d.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            desc_2d.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            desc_2d.fill_mode = SDL_GPU_FILLMODE_FILL;
            desc_2d.cull_mode = SDL_GPU_CULLMODE_NONE;
            desc_2d.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
            desc_2d.vertex_shader = vertex_shader_2d.handle;
            desc_2d.fragment_shader = fragment_shader_2d.handle;
            desc_2d.enable_depth_test = false;
            desc_2d.enable_depth_write = false;
            desc_2d.enable_blend = false;
            desc_2d.vertex_buffer_descriptions = &vertex_buffer_desc;
            desc_2d.num_vertex_buffers = 1;
            desc_2d.vertex_attributes = vertex_shader_2d.reflection_info.vertex_attributes;
            desc_2d.num_vertex_attributes = vertex_shader_2d.reflection_info.vertex_attribute_count;
            desc_2d.num_samplers = 0;

            two_dimension_pipeline = graphics_pipeline_create(window.device, &desc_2d);
            if (!two_dimension_pipeline)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create 2D pipeline");
            }
            shader_release(window.device, &vertex_shader_2d);
            shader_release(window.device, &fragment_shader_2d);

            // Create composite pipeline
            Shader vertex_shader = shader_create(window.device,
                SDL_GPU_SHADERSTAGE_VERTEX, "Resources/Shaders/composite.vert.spv",
                "main");

            Shader fragment_shader = shader_create(window.device,
                SDL_GPU_SHADERSTAGE_FRAGMENT, "Resources/Shaders/composite.frag.spv",
                "main");

            if (vertex_shader.handle && fragment_shader.handle)
            {
                GraphicsPipelineDescription desc = {0};
                desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
                desc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
                desc.format = window.swapchain_format;
                desc.fill_mode = SDL_GPU_FILLMODE_FILL;
                desc.cull_mode = SDL_GPU_CULLMODE_NONE;
                desc.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
                desc.vertex_shader = vertex_shader.handle;
                desc.fragment_shader = fragment_shader.handle;
                desc.enable_depth_test = false;
                desc.enable_depth_write = false;
                desc.enable_blend = false;
                desc.vertex_buffer_descriptions = NULL;
                desc.num_vertex_buffers = 0;
                desc.vertex_attributes = NULL;
                desc.num_vertex_attributes = 0;
                desc.num_samplers = 1;

                composite_pipeline = graphics_pipeline_create(window.device, &desc);
                if (!composite_pipeline)
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create composite pipeline: %s", SDL_GetError());
                }
            }
            
            shader_release(window.device, &vertex_shader);
            shader_release(window.device, &fragment_shader);
        }
        
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(window.device);
        Swapchain swapchain = {0};
        if (!swapchain_acquire(cmd, &window, &swapchain))
        {
            SDL_CancelGPUCommandBuffer(cmd);
            break;
        }

        // First pass: Render 2D quad to scene render target
        {
            SDL_GPUColorTargetInfo scene_target_info = {0};
            scene_target_info.texture = scene_texture;
            scene_target_info.clear_color.r = 0.1f;
            scene_target_info.clear_color.g = 0.1f;
            scene_target_info.clear_color.b = 0.1f;
            scene_target_info.clear_color.a = 1.0f;
            scene_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            scene_target_info.store_op = SDL_GPU_STOREOP_STORE;
            scene_target_info.cycle = false;

            SDL_GPURenderPass *scene_pass = SDL_BeginGPURenderPass(cmd, &scene_target_info, 1, NULL);
            if (scene_pass && two_dimension_pipeline)
            {
                SDL_BindGPUGraphicsPipeline(scene_pass, two_dimension_pipeline);
                
                // Bind uniform buffer
                SDL_GPUBufferBinding uniform_binding = {0};
                uniform_binding.buffer = view_projection_buffer.buffer;
                uniform_binding.offset = 0;
                SDL_BindGPUVertexStorageBuffers(scene_pass, 0, &uniform_binding, 1);
                
                // Build batch of quads
                batch_renderer_2d_begin(&batch_renderer);
                
                // Example: Add a few test quads
                batch_renderer_2d_add_quad(&batch_renderer, 
                    (Vector2f){0.0f, 0.0f}, (Vector2f){1.0f, 1.0f}, 
                    (Vector4f){1.0f, 1.0f, 0.0f, 1.0f});
                
                batch_renderer_2d_add_quad(&batch_renderer, 
                    (Vector2f){2.0f, 0.0f}, (Vector2f){0.8f, 0.8f}, 
                    (Vector4f){1.0f, 0.0f, 0.0f, 1.0f});
                
                batch_renderer_2d_add_quad(&batch_renderer, 
                    (Vector2f){-2.0f, 0.0f}, (Vector2f){0.6f, 0.6f}, 
                    (Vector4f){0.0f, 1.0f, 1.0f, 1.0f});
                
                batch_renderer_2d_end(&batch_renderer);
                
                // Draw all batched quads
                batch_renderer_2d_draw(&batch_renderer, scene_pass);
                
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
            if (render_pass && composite_pipeline)
            {
                SDL_BindGPUGraphicsPipeline(render_pass, composite_pipeline);
                
                SDL_GPUTextureSamplerBinding texture_binding = {0};
                texture_binding.texture = scene_texture;
                texture_binding.sampler = sampler;
                
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

    SDL_WaitForGPUIdle(window.device);

    graphics_pipeline_destroy(window.device, composite_pipeline);
    graphics_pipeline_destroy(window.device, two_dimension_pipeline);
    
    batch_renderer_2d_destroy(&batch_renderer);
    uniform_buffer_destroy(window.device, &view_projection_buffer);
    
    SDL_ReleaseGPUSampler(window.device, sampler);
    SDL_ReleaseGPUTexture(window.device, scene_texture);
    SDL_Quit();

    return 0;
}