#ifndef PIPELINE_H
#define PIPELINE_H

#include <SDL3/SDL_gpu.h>

SDL_GPUComputePipeline* CreateQuadComputePipeline(SDL_GPUDevice* device, const char* cs_filename);
SDL_GPUGraphicsPipeline* CreateQuadGraphicsPipeline(SDL_GPUDevice* device,
                                                    SDL_Window* window,
                                                    const char* vs_filename,
                                                    const char* fs_filename);

#define QUAD_COMPUTE_THREAD_COUNT_X 64
#define QUAD_COMPUTE_THREAD_COUNT_Y 1
#define QUAD_COMPUTE_THREAD_COUNT_Z 1
#endif /* PIPELINE_H */
