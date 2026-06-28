#include "shader.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, ShaderOptions options) {
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
  void* code_data = SDL_LoadFile(shader_rel_filename, &code_size);
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
  SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_create_info);
  SDL_Log("Loaded shader: %s", shader_rel_filename);
  SDL_free(code_data);
  return shader;
}
