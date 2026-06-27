#ifndef QUAD_GROUP_H
#define QUAD_GROUP_H

#include "quad.h"

typedef struct {
  float position[2];
} QuadInstanceData;

typedef struct {
  float origin[2];
  float angle;
  float speed;
  float radius;
} QuadInstanceState;

typedef struct {
  SingleQuad* single_quad;
  size_t instance_count;
  Uint64 last_tick;
  QuadInstanceData* instances;
  QuadInstanceState* states;
  SDL_GPUBuffer* buffer;
} QuadGroup;

QuadGroup* CreateQuadGroup(SDL_GPUDevice* device, size_t instance_count);
void DestroyQuadGroup(QuadGroup* group, SDL_GPUDevice* device);
void UploadQuadGroupStatic(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
void UploadQuadGroupFrame(QuadGroup* group, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
void UpdateQuadGroup(QuadGroup* group);
void RenderQuadGroup(QuadGroup* group,
                     SDL_GPUCommandBuffer* cmdbuf,
                     SDL_GPURenderPass* render_pass);
#endif /* QUAD_GROUP_H */
