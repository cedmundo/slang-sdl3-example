#ifndef SHADER_H
#define SHADER_H

#include <SDL3/SDL_gpu.h>

typedef struct {
  const char* filename;
  Uint32 sampler_count;
  Uint32 uniform_buffer_count;
  Uint32 storage_buffer_count;
  Uint32 storage_texture_count;
  SDL_GPUShaderStage stage;
} ShaderOptions;

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, ShaderOptions options);
#endif /* SHADER_H */
