#include "texture.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>

#include <SDL3_image/SDL_image.h>

Texture* CreateTextureFromImage(const char* image, SDL_GPUDevice* device) {
  Texture* texture = SDL_malloc(sizeof(Texture));
  if (texture == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  char image_path[255] = {0};
  SDL_strlcat(image_path, SDL_GetBasePath(), 255);
  SDL_strlcat(image_path, image, 255);

  SDL_Surface* original = IMG_Load(image_path);
  if (original == NULL) {
    DestroyTexture(texture, device);
    return NULL;
  }

  texture->surface = SDL_ConvertSurface(original, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(original);

  if (texture->surface == NULL) {
    DestroyTexture(texture, device);
    return NULL;
  }
  texture->surface_pixels_size = texture->surface->pitch * texture->surface->h;

  SDL_GPUTextureCreateInfo texture_create_info = {0};
  texture_create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  texture_create_info.width = texture->surface->w;
  texture_create_info.height = texture->surface->h;
  texture_create_info.num_levels = 1;
  texture_create_info.layer_count_or_depth = 1;
  texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
  texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  texture->texture = SDL_CreateGPUTexture(device, &texture_create_info);

  SDL_GPUSamplerCreateInfo sampler_create_info = {0};
  sampler_create_info.min_lod = 0;
  sampler_create_info.max_lod = 1;
  texture->sampler = SDL_CreateGPUSampler(device, &sampler_create_info);

  texture->sampler_binding = (SDL_GPUTextureSamplerBinding){
      .sampler = texture->sampler,
      .texture = texture->texture,
  };
  return texture;
}

void DestroyTexture(Texture* texture, SDL_GPUDevice* device) {
  if (texture == NULL) {
    return;
  }

  if (texture->surface != NULL) {
    SDL_DestroySurface(texture->surface);
  }

  if (texture->texture != NULL) {
    SDL_ReleaseGPUTexture(device, texture->texture);
  }

  if (texture->sampler != NULL) {
    SDL_ReleaseGPUSampler(device, texture->sampler);
  }

  SDL_free(texture);
}

void UploadTexture(Texture* texture, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
  SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {0};
  transfer_buffer_create_info.size = texture->surface_pixels_size;
  transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer* transfer_buffer =
      SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);

  void* gpu_staging = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
  { SDL_memcpy(gpu_staging, texture->surface->pixels, texture->surface_pixels_size); }
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  SDL_GPUTextureTransferInfo src = {0};
  src.transfer_buffer = transfer_buffer;
  src.pixels_per_row = texture->surface->pitch / 4;
  src.rows_per_layer = texture->surface->h;

  SDL_GPUTextureRegion dst = {0};
  dst.texture = texture->texture;
  dst.h = texture->surface->h;
  dst.w = texture->surface->w;
  dst.d = 1;
  SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}
