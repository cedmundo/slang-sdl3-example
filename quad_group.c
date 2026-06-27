#include "quad_group.h"
#include "quad.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

QuadGroup* CreateQuadGroup(SDL_GPUDevice* device, size_t instance_count) {
  QuadGroup* group = SDL_malloc(sizeof(QuadGroup));
  if (group == NULL) {
    return NULL;
  }

  size_t instance_data_buf_size = sizeof(QuadInstanceData) * instance_count;
  group->instance_count = instance_count;
  group->single_quad = CreateSingleQuad(device);
  if (group->single_quad == NULL) {
    DestroyQuadGroup(group, device);
    return NULL;
  }

  SDL_GPUBufferCreateInfo buffer_create_info = {0};
  buffer_create_info.size = instance_data_buf_size;
  buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
  group->buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
  group->last_tick = SDL_GetTicks();

  group->instances = SDL_malloc(instance_data_buf_size);
  if (group->instances == NULL) {
    DestroyQuadGroup(group, device);
    return NULL;
  }

  group->states = SDL_malloc(sizeof(QuadInstanceState) * instance_count);
  if (group->states == NULL) {
    DestroyQuadGroup(group, device);
    return NULL;
  }

  // Randomize staring positions
  for (size_t i = 0; i < instance_count; i++) {
    float x = SDL_randf() * 0.8f - 0.5f;
    float y = SDL_randf() * 2.0f - 0.5f;
    float r = SDL_randf() * 0.3f - 0.2f;
    float s = SDL_randf() + 0.4f;

    group->states[i].origin[0] = x;
    group->states[i].origin[1] = y;
    group->states[i].speed = s;
    group->states[i].angle = 0.0f;
    group->states[i].radius = r;

    group->instances[i].position[0] = x;
    group->instances[i].position[1] = y;
  }

  return group;
}

void DestroyQuadGroup(QuadGroup* group, SDL_GPUDevice* device) {
  if (group == NULL) {
    return;
  }

  if (group->buffer != NULL) {
    SDL_ReleaseGPUBuffer(device, group->buffer);
  }

  if (group->instances != NULL) {
    SDL_free(group->instances);
  }

  if (group->states != NULL) {
    SDL_free(group->states);
  }

  if (group->single_quad != NULL) {
    DestroySingleQuad(group->single_quad, device);
  }

  SDL_free(group);
}

void UploadQuadGroupStatic(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
  UploadSingleQuad(group->single_quad, device, copy_pass);
}

void UploadQuadGroupFrame(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
  size_t instance_data_buf_size = sizeof(QuadInstanceData) * group->instance_count;

  SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {0};
  transfer_buffer_create_info.size = instance_data_buf_size;
  transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer* transfer_buffer =
      SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);

  void* gpu_staging = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
  SDL_memcpy(gpu_staging, group->instances, instance_data_buf_size);
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  SDL_GPUTransferBufferLocation src = {
      .transfer_buffer = transfer_buffer,
      .offset = 0,
  };

  SDL_GPUBufferRegion dst = {
      .buffer = group->buffer,
      .offset = 0,
      .size = instance_data_buf_size,
  };

  SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

void UpdateQuadGroup(QuadGroup* group) {
  UpdateSingleQuad(group->single_quad);

  Uint64 cur_tick = SDL_GetTicks();
  Uint64 delta_ticks = cur_tick - group->last_tick;
  float delta_time = (float)delta_ticks / 1000.0;
  group->last_tick = cur_tick;

  for (size_t i = 0; i < group->instance_count; i++) {
    QuadInstanceState* state = &group->states[i];
    float angle = state->angle;
    state->angle = angle + delta_time * state->speed;
    group->instances[i].position[0] = state->origin[0] + (SDL_cos(angle) * state->radius);
    group->instances[i].position[1] = state->origin[1] + (SDL_sin(angle) * state->radius);
  }
}

void RenderQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPURenderPass* render_pass) {
  RenderSingleQuad(group->single_quad, cmdbuf, render_pass, &group->buffer, 1,
                   group->instance_count);
}
