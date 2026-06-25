#include "quad.h"
#include "texture.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

static const float v_pos_buffer[][3] = {
    {+0.5, +0.5, 0},
    {+0.5, -0.5, 0},
    {-0.5, +0.5, 0},
    {-0.5, -0.5, 0},
};

static const float v_col_buffer[][3] = {
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 1.0, 1.0},
};

static const float v_uvs_buffer[][2] = {
    {0.0, 0.0},
    {0.0, 1.0},
    {1.0, 0.0},
    {1.0, 1.0},
};

// IMPORTANT: Either use U32 or U16
static const Uint32 quad_indices[] = {
    0, 1, 2, 2, 1, 3,
};

#define V_INDEX_COUNT (sizeof(quad_indices) / sizeof(unsigned))
#define QUAD_BUFFER_SIZE \
  (sizeof(v_pos_buffer) + sizeof(v_col_buffer) + sizeof(v_uvs_buffer) + sizeof(quad_indices))

#define V_POS_BINDING_IDX (0)
#define V_COL_BINDING_IDX (1)
#define V_UVS_BINDING_IDX (2)
#define INDICES_BINDING_IDX (3)

SingleQuad* CreateSingleQuad(SDL_GPUDevice* device) {
  SingleQuad* quad = SDL_malloc(sizeof(SingleQuad));
  if (quad == NULL) {
    return quad;
  }

  // This is going to be used to hold the data in GPU
  SDL_GPUBufferCreateInfo buffer_create_info = {0};
  buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ |
                             SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_INDEX;
  buffer_create_info.size = QUAD_BUFFER_SIZE;

  quad->buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
  if (quad->buffer == NULL) {
    DestroySingleQuad(quad, device);
    return NULL;
  }

  // This is going to be used to transfer the data into GPU
  SDL_GPUTransferBufferCreateInfo create_info = {0};
  create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  create_info.size = QUAD_BUFFER_SIZE;
  quad->transfer_buffer = SDL_CreateGPUTransferBuffer(device, &create_info);
  if (quad->transfer_buffer == NULL) {
    DestroySingleQuad(quad, device);
    return NULL;
  }

  // First part of buffer is positions:
  quad->buffer_bindings[V_POS_BINDING_IDX] = (SDL_GPUBufferBinding){
      .buffer = quad->buffer,
      .offset = 0,
  };

  // Second part of buffer is colors:
  quad->buffer_bindings[V_COL_BINDING_IDX] = (SDL_GPUBufferBinding){
      .buffer = quad->buffer,
      .offset = sizeof(v_pos_buffer),
  };

  // Third part of buffer is uvs:
  quad->buffer_bindings[V_UVS_BINDING_IDX] = (SDL_GPUBufferBinding){
      .buffer = quad->buffer,
      .offset = sizeof(v_pos_buffer) + sizeof(v_col_buffer),
  };

  // Foruth part of buffer are indices:
  quad->buffer_bindings[INDICES_BINDING_IDX] = (SDL_GPUBufferBinding){
      .buffer = quad->buffer,
      .offset = sizeof(v_pos_buffer) + sizeof(v_col_buffer) + sizeof(v_uvs_buffer),
  };

  quad->texture = CreateTextureFromImage("quad.png", device);
  if (quad->texture == NULL) {
    DestroySingleQuad(quad, device);
    return NULL;
  }

  quad->indices_count = V_INDEX_COUNT;
  quad->uploaded = false;
  return quad;
}

void DestroySingleQuad(SingleQuad* quad, SDL_GPUDevice* device) {
  if (quad == NULL) {
    return;
  }

  if (quad->buffer != NULL) {
    SDL_ReleaseGPUBuffer(device, quad->buffer);
  }

  if (quad->transfer_buffer != NULL) {
    SDL_ReleaseGPUTransferBuffer(device, quad->transfer_buffer);
  }

  if (quad->texture != NULL) {
    DestroyTexture(quad->texture, device);
  }

  SDL_free(quad);
}

void UploadSingleQuad(SingleQuad* quad, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
  if (quad->uploaded) {
    return;
  }

  void* gpu_staging = SDL_MapGPUTransferBuffer(device, quad->transfer_buffer, false);
  {
    SDL_memset(gpu_staging, 0, QUAD_BUFFER_SIZE);

    // buffer + 0: positions
    SDL_memcpy(gpu_staging, v_pos_buffer, sizeof(v_pos_buffer));

    // buffer + v_pos: colors
    SDL_memcpy(gpu_staging + sizeof(v_pos_buffer), v_col_buffer, sizeof(v_col_buffer));

    // buffer + v_pos + v_col: v_uvs
    SDL_memcpy(gpu_staging + sizeof(v_pos_buffer) + sizeof(v_col_buffer), v_uvs_buffer,
               sizeof(v_uvs_buffer));

    // buffer + v_pos + v_col + v_uvs: indices
    SDL_memcpy(gpu_staging + sizeof(v_pos_buffer) + sizeof(v_col_buffer) + sizeof(v_uvs_buffer),
               quad_indices, sizeof(quad_indices));
  }
  SDL_UnmapGPUTransferBuffer(device, quad->transfer_buffer);

  SDL_GPUTransferBufferLocation src = {
      .transfer_buffer = quad->transfer_buffer,
      .offset = 0,
  };

  SDL_GPUBufferRegion dst = {
      .buffer = quad->buffer,
      .offset = 0,
      .size = QUAD_BUFFER_SIZE,
  };

  SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);

  // upload the texture
  UploadTexture(quad->texture, device, copy_pass);
  quad->uploaded = true;
}

void UpdateSingleQuad(SingleQuad* quad) {
  quad->frag_uniforms.time = (float)SDL_GetTicks() / 1000.0;
}

void RenderSingleQuad(SingleQuad* quad,
                      SDL_GPUCommandBuffer* cmdbuf,
                      SDL_GPURenderPass* render_pass) {
  SDL_BindGPUVertexBuffers(render_pass, 0, quad->buffer_bindings, 3);
  SDL_BindGPUIndexBuffer(render_pass, &quad->buffer_bindings[INDICES_BINDING_IDX],
                         SDL_GPU_INDEXELEMENTSIZE_32BIT);
  SDL_PushGPUFragmentUniformData(cmdbuf, 0, &quad->frag_uniforms, sizeof(QuadFUniformData));
  SDL_BindGPUFragmentSamplers(render_pass, 0, &quad->texture->sampler_binding, 1);
  SDL_DrawGPUIndexedPrimitives(render_pass, quad->indices_count, 1, 0, 0, 1);
}
