#ifndef QUAD_GROUP_H
#define QUAD_GROUP_H

#include <SDL3/SDL_gpu.h>
#include "quad.h"

typedef struct {
  float position[2];
} QuadInstanceData;

typedef struct {
  float origin[2];
  float angle;
  float speed;
  float radius;
  float _padding0;
} QuadInstanceState;

typedef struct {
  float delta_time;
  Uint32 span_start;
  Uint32 span_end;
  float _padding0;
} UniformCData;

typedef struct {
  SingleQuad* single_quad;
  size_t instance_count;
  Uint64 last_tick;
  size_t workgroup_size;
  size_t required_workgroups;
  SDL_GPUBuffer* buffers[2];
} QuadGroup;

QuadGroup* CreateQuadGroup(SDL_GPUDevice* device, size_t instance_count);
void DestroyQuadGroup(QuadGroup* group, SDL_GPUDevice* device);
void UploadQuadGroupStatic(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
void UpdateQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPUComputePipeline* pipeline);
void RenderQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPURenderPass* render_pass);
#endif /* QUAD_GROUP_H */
