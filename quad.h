#ifndef QUAD_H
#define QUAD_H

#include <SDL3/SDL_gpu.h>

#include "texture.h"

typedef struct {
  float time;
} QuadFUniformData;

typedef struct {
  SDL_GPUBuffer* buffer;
  SDL_GPUTransferBuffer* transfer_buffer;
  SDL_GPUBufferBinding buffer_bindings[4];
  Uint32 indices_count;
  QuadFUniformData frag_uniforms;
  Texture* texture;
  bool uploaded;
} SingleQuad;

SingleQuad* CreateSingleQuad(SDL_GPUDevice* device);
void DestroySingleQuad(SingleQuad* quad, SDL_GPUDevice* device);
void UploadSingleQuad(SingleQuad* quad, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
void UpdateSingleQuad(SingleQuad* quad);
void RenderSingleQuad(SingleQuad* quad,
                      SDL_GPUCommandBuffer* cmdbuf,
                      SDL_GPURenderPass* render_pass,
                      SDL_GPUBuffer** storage_buffers,
                      size_t storage_buffers_count,
                      size_t instances_count);
#endif /* QUAD_H */
