#include "quad_group.h"
#include "pipeline.h"
#include "quad.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#define BUFFER_INDEX_STATES (0)
#define BUFFER_INDEX_POSITIONS (1)

static float v_remap(float value, float high1, float low1, float high2, float low2);

QuadGroup* CreateQuadGroup(SDL_GPUDevice* device, size_t instance_count) {
  QuadGroup* group = SDL_malloc(sizeof(QuadGroup));
  if (group == NULL) {
    return NULL;
  }

  group->last_tick = SDL_GetTicks();
  group->instance_count = instance_count;
  group->single_quad = CreateSingleQuad(device);
  if (group->single_quad == NULL) {
    DestroyQuadGroup(group, device);
    return NULL;
  }

  group->workgroup_size = QUAD_COMPUTE_THREAD_COUNT_X;
  group->required_workgroups =
      (size_t)SDL_ceil((double)group->instance_count / group->workgroup_size);

  // We want two buffers: first one is for states, second one to pass it to shaders
  size_t states_buf_size = sizeof(QuadInstanceState) * instance_count;
  SDL_GPUBufferCreateInfo states_buffer_create_info = {0};
  states_buffer_create_info.size = states_buf_size;
  states_buffer_create_info.usage =
      SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
  group->buffers[BUFFER_INDEX_STATES] = SDL_CreateGPUBuffer(device, &states_buffer_create_info);

  // This buffer we want compute to write AND also read from graphics
  size_t positions_buf_size = sizeof(QuadInstanceData) * instance_count;
  SDL_GPUBufferCreateInfo positions_buffer_create_info = {0};
  positions_buffer_create_info.size = positions_buf_size;
  positions_buffer_create_info.usage =
      SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
  group->buffers[BUFFER_INDEX_POSITIONS] =
      SDL_CreateGPUBuffer(device, &positions_buffer_create_info);

  return group;
}

void DestroyQuadGroup(QuadGroup* group, SDL_GPUDevice* device) {
  if (group == NULL) {
    return;
  }

  SDL_ReleaseGPUBuffer(device, group->buffers[BUFFER_INDEX_STATES]);
  SDL_ReleaseGPUBuffer(device, group->buffers[BUFFER_INDEX_POSITIONS]);

  if (group->single_quad != NULL) {
    DestroySingleQuad(group->single_quad, device);
  }

  SDL_free(group);
}

void UploadQuadGroupStatic(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
  // Upload the single quad mesh and texture data
  UploadSingleQuad(group->single_quad, device, copy_pass);

  // Initialize initial states
  size_t instance_count = group->instance_count;
  size_t states_buffer_size = sizeof(QuadInstanceState) * group->instance_count;
  QuadInstanceState* states = SDL_aligned_alloc(16, states_buffer_size);
  if (states == NULL) {
    SDL_Log("Error: could not create temp buffer to upload initialization data");
    return;
  }

  // Randomize staring state
  for (size_t i = 0; i < instance_count; i++) {
    float x = v_remap(SDL_randf(), 1.0f, 0.0f, 0.8f, -0.8f);
    float y = v_remap(SDL_randf(), 1.0f, 0.0f, 0.8f, -0.8f);
    float r = v_remap(SDL_randf(), 1.0f, 0.0f, 0.1f, 0.3f);
    float s = SDL_randf() > 0.5 ? -0.4f : 0.4f;

    states[i].origin[0] = x;
    states[i].origin[1] = y;
    states[i].speed = s;
    states[i].angle = 0.0f;
    states[i].radius = r;
  }

  // Copy starting states to the first buffer
  SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {0};
  transfer_buffer_create_info.size = states_buffer_size;
  transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer* transfer_buffer =
      SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);

  void* gpu_staging = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
  SDL_memcpy(gpu_staging, states, states_buffer_size);
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  SDL_GPUTransferBufferLocation src = {
      .transfer_buffer = transfer_buffer,
      .offset = 0,
  };

  SDL_GPUBufferRegion dst = {
      .buffer = group->buffers[0],
      .offset = 0,
      .size = states_buffer_size,
  };

  SDL_aligned_free(states);
  SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

void UpdateQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPUComputePipeline* pipeline) {
  UpdateSingleQuad(group->single_quad);

  Uint64 cur_tick = SDL_GetTicks();
  Uint64 delta_ticks = cur_tick - group->last_tick;
  float delta_time = (float)delta_ticks / 1000.0;
  group->last_tick = cur_tick;

  SDL_GPUStorageBufferReadWriteBinding buffer_bindings[2] = {
      (SDL_GPUStorageBufferReadWriteBinding){
          .buffer = group->buffers[BUFFER_INDEX_STATES],
          .cycle = false,
      },
      (SDL_GPUStorageBufferReadWriteBinding){
          .buffer = group->buffers[BUFFER_INDEX_POSITIONS],
          .cycle = false,
      },
  };

  SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(cmdbuf, NULL, 0, buffer_bindings, 2);
  {
    SDL_BindGPUComputePipeline(compute_pass, pipeline);
    SDL_BindGPUComputeStorageBuffers(compute_pass, 0, group->buffers, 2);

    const SDL_ALIGNED(16) UniformCData uniforms = {
        .delta_time = delta_time,
        .span_start = 0,
        .span_end = (Uint32)group->instance_count,
    };

    SDL_PushGPUComputeUniformData(cmdbuf, 0, &uniforms, sizeof(UniformCData));
    SDL_DispatchGPUCompute(compute_pass, (Uint32)group->required_workgroups, 1, 1);
  }
  SDL_EndGPUComputePass(compute_pass);
}

void RenderQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPURenderPass* render_pass) {
  // We pass only the last buffer since it is where positions are stored
  RenderSingleQuad(group->single_quad, cmdbuf, render_pass, &group->buffers[BUFFER_INDEX_POSITIONS],
                   1, group->instance_count);
}

static float v_remap(float value, float high1, float low1, float high2, float low2) {
  return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
}
