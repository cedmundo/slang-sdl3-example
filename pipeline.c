#include "pipeline.h"
#include "shader.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

SDL_GPUComputePipeline* CreateQuadComputePipeline(SDL_GPUDevice* device, const char* cs_filename) {
  char cs_path[255] = {0};
  SDL_strlcat(cs_path, SDL_GetBasePath(), 255);
  SDL_strlcat(cs_path, cs_filename, 255);

  size_t code_len = 0;
  Uint8* code_buf = SDL_LoadFile(cs_path, &code_len);
  if (code_buf == NULL) {
    return NULL;
  }

  SDL_GPUComputePipelineCreateInfo create_info = {0};
  create_info.code = code_buf;
  create_info.code_size = code_len;
  create_info.num_uniform_buffers = 1;            // time and quad count
  create_info.num_readwrite_storage_buffers = 2;  // states and positions
  create_info.entrypoint = "main";
  create_info.threadcount_x = QUAD_COMPUTE_THREAD_COUNT_X;
  create_info.threadcount_y = QUAD_COMPUTE_THREAD_COUNT_Y;
  create_info.threadcount_z = QUAD_COMPUTE_THREAD_COUNT_Z;
  create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
  SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &create_info);
  SDL_free(code_buf);
  return pipeline;
}

SDL_GPUGraphicsPipeline* CreateQuadGraphicsPipeline(SDL_GPUDevice* device,
                                                    SDL_Window* window,
                                                    const char* vs_filename,
                                                    const char* fs_filename) {
  SDL_GPUGraphicsPipeline* pipeline = NULL;

  ShaderOptions vert_options = {0};
  vert_options.filename = vs_filename;
  vert_options.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  vert_options.storage_buffer_count = 1;
  SDL_GPUShader* vert_shader = LoadShader(device, vert_options);
  if (vert_shader == NULL) {
    SDL_Log("Error: failed to load shader: %s %s", vert_options.filename, SDL_GetError());
    goto terminate;
  }

  ShaderOptions frag_options = {0};
  frag_options.filename = fs_filename;
  frag_options.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  frag_options.uniform_buffer_count = 1;
  frag_options.sampler_count = 1;
  SDL_GPUShader* frag_shader = LoadShader(device, frag_options);
  if (frag_shader == NULL) {
    SDL_Log("Error: failed to load shader: %s %s", frag_options.filename, SDL_GetError());
    goto terminate;
  }

  // standard blending for this shader
  SDL_GPUColorTargetBlendState blend_state = {0};
  blend_state.enable_blend = true;
  blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
  blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
  blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

  // color configuration with blend state
  SDL_GPUColorTargetDescription color_desc = {0};
  color_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window);
  color_desc.blend_state = blend_state;

  // this are the targets to render (the swapchain texture)
  SDL_GPUGraphicsPipelineTargetInfo color_target_info = {0};
  color_target_info.num_color_targets = 1;
  color_target_info.color_target_descriptions = (SDL_GPUColorTargetDescription[]){color_desc};

  // finally we can create the actual pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {0};
  pipeline_create_info.target_info = color_target_info;
  pipeline_create_info.fragment_shader = frag_shader;
  pipeline_create_info.vertex_shader = vert_shader;
  pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  pipeline_create_info.vertex_input_state = (SDL_GPUVertexInputState){
      .num_vertex_attributes = 3,
      .vertex_attributes =
          (SDL_GPUVertexAttribute[]){
              {.buffer_slot = 0,
               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
               .location = 0,
               .offset = 0},
              {.buffer_slot = 1,
               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
               .location = 1,
               .offset = 0},
              {.buffer_slot = 2,
               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
               .location = 2,
               .offset = 0},
          },
      .num_vertex_buffers = 3,
      .vertex_buffer_descriptions =
          (SDL_GPUVertexBufferDescription[]){
              {.slot = 0,
               .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
               .instance_step_rate = 0,
               .pitch = sizeof(float) * 3},
              {.slot = 1,
               .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
               .instance_step_rate = 0,
               .pitch = sizeof(float) * 3},
              {.slot = 2,
               .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
               .instance_step_rate = 0,
               .pitch = sizeof(float) * 2},
          },
  };
  pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

terminate:
  if (vert_shader != NULL) {
    SDL_ReleaseGPUShader(device, vert_shader);
  }

  if (frag_shader != NULL) {
    SDL_ReleaseGPUShader(device, frag_shader);
  }

  return pipeline;
}
