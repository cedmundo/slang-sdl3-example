#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

typedef struct {
  SDL_Surface *surface;
  size_t surface_pixels_size;
  SDL_GPUTexture *texture;
  SDL_GPUSampler *sampler;
  SDL_GPUTextureSamplerBinding sampler_binding;
} Texture;

Texture *CreateTextureFromImage(const char *image, SDL_GPUDevice *device);
void DestroyTexture(Texture *texture, SDL_GPUDevice *device);

void UploadTexture(Texture *texture, SDL_GPUDevice *device,
                   SDL_GPUCopyPass *copy_pass);
#endif /* TEXTURE_H */
