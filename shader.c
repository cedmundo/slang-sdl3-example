#include "shader.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, ShaderOptions options) {
  SDL_assert(device != NULL);

  char shader_rel_filename[255] = {0};
  SDL_strlcat(shader_rel_filename, SDL_GetBasePath(), 255);
  SDL_strlcat(shader_rel_filename, options.filename, 255);

  SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(device);
  if (!(supported_formats & SDL_GPU_SHADERFORMAT_SPIRV)) {
    SDL_Log("Error: GPU device doesn't support SPIR-V shader format");
    return NULL;
  }

  size_t code_size;
  void *code_data = SDL_LoadFile(shader_rel_filename, &code_size);
  if (code_data == NULL) {
    SDL_Log("Couldn't load shader code: %s", SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_create_info = {
      .code = code_data,
      .code_size = code_size,
      .entrypoint = "main",
      .stage = options.stage,
      .format = SDL_GPU_SHADERFORMAT_SPIRV,
      .num_samplers = options.sampler_count,
      .num_uniform_buffers = options.uniform_buffer_count,
      .num_storage_buffers = options.storage_buffer_count,
      .num_storage_textures = options.storage_texture_count,
  };
  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_create_info);
  SDL_Log("Loaded shader: %s", shader_rel_filename);
  SDL_free(code_data);
  return shader;
}

SDL_GPUGraphicsPipeline *CreatePipeline(SDL_GPUDevice *device,
                                        SDL_Window *window,
                                        ShaderOptions vert_options,
                                        ShaderOptions frag_options) {
  SDL_GPUGraphicsPipeline *pipeline = NULL;
  SDL_GPUShader *vert_shader = LoadShader(device, vert_options);
  if (vert_shader == NULL) {
    SDL_Log("Error: failed to load shader: %s %s", vert_options.filename,
            SDL_GetError());
    goto terminate;
  }

  SDL_GPUShader *frag_shader = LoadShader(device, frag_options);
  if (frag_shader == NULL) {
    SDL_Log("Error: failed to load shader: %s %s", frag_options.filename,
            SDL_GetError());
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
  color_target_info.color_target_descriptions =
      (SDL_GPUColorTargetDescription[]){color_desc};

  // finally we can create the actual pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {0};
  pipeline_create_info.target_info = color_target_info;
  pipeline_create_info.fragment_shader = frag_shader;
  pipeline_create_info.vertex_shader = vert_shader;
  pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  pipeline_create_info.vertex_input_state = (SDL_GPUVertexInputState){
      .num_vertex_attributes = 2,
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
          },
      .num_vertex_buffers = 2,
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
