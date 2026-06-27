#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "quad_group.h"
#include "shader.h"

#define WINDOW_TITLE "Slang + SDLGPU Example"
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 500
#define VK_MAKE_API_VERSION(variant, major, minor, patch)                                         \
  ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | \
   ((uint32_t)(patch)))

static const size_t vk_device_extension_count = 1;
static const char* vk_device_extension_names[] = {
    "VK_KHR_shader_draw_parameters",
};

typedef struct {
  SDL_Window* window;
  SDL_GPUDevice* device;
  SDL_GPUViewport viewport;

  // our resources
  SDL_GPUGraphicsPipeline* flat_color_pipeline;
  QuadGroup* quad_group;
} ExampleApp;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Error: SDL_Init(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  ExampleApp* app = SDL_malloc(sizeof(ExampleApp));
  if (!app) {
    SDL_OutOfMemory();
    return SDL_APP_FAILURE;
  }

  *appstate = app;
  app->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (app->window == NULL) {
    SDL_Log("Error: SDL_CreateWindow(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  app->viewport = (SDL_GPUViewport){.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};

  SDL_GPUVulkanOptions vulkan_options = {0};
  vulkan_options.vulkan_api_version = VK_MAKE_API_VERSION(0, 1, 4, 0);
  vulkan_options.device_extension_count = vk_device_extension_count;
  vulkan_options.device_extension_names = vk_device_extension_names;

  SDL_PropertiesID device_properties = SDL_CreateProperties();
  SDL_SetBooleanProperty(device_properties, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
  SDL_SetBooleanProperty(device_properties, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
  SDL_SetBooleanProperty(device_properties,
                         SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN,
                         true);
  SDL_SetPointerProperty(device_properties, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER,
                         &vulkan_options);

  app->device = SDL_CreateGPUDeviceWithProperties(device_properties);
  if (app->device == NULL) {
    SDL_Log("Error: SDL_CreateGPUDevice(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(app->device, app->window)) {
    SDL_Log("Error: SDL_ClaimWindowForGPUDevice(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // create resources
  ShaderOptions vert_shader_opts = {0};
  vert_shader_opts.filename = "flat-color.vs.spirv";
  vert_shader_opts.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  vert_shader_opts.storage_buffer_count = 1;

  ShaderOptions frag_shader_opts = {0};
  frag_shader_opts.filename = "flat-color.fs.spirv";
  frag_shader_opts.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  frag_shader_opts.uniform_buffer_count = 1;
  frag_shader_opts.sampler_count = 1;

  app->flat_color_pipeline =
      CreatePipeline(app->device, app->window, vert_shader_opts, frag_shader_opts);
  if (app->flat_color_pipeline == NULL) {
    SDL_Log("Error: failed to create pipeline: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  app->quad_group = CreateQuadGroup(app->device, 50);
  if (app->quad_group == NULL) {
    SDL_Log("Error: failed to create group: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Initial upload (static data):
  SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(app->device);
  if (cmdbuf == NULL) {
    SDL_Log("Error: failed to get initial command buffer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUCopyPass* static_data_copy_pass = SDL_BeginGPUCopyPass(cmdbuf);
  {
    if (static_data_copy_pass == NULL) {
      SDL_Log("Error: failed to get initial copy pass: %s", SDL_GetError());
      return SDL_APP_FAILURE;
    }

    UploadQuadGroupStatic(app->quad_group, app->device, static_data_copy_pass);
  }
  SDL_EndGPUCopyPass(static_data_copy_pass);
  SDL_SubmitGPUCommandBuffer(cmdbuf);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  ExampleApp* app = (ExampleApp*)appstate;
  SDL_assert(app != NULL);

  // Update everything, including camera, positions, etc...
  UpdateQuadGroup(app->quad_group);

  // Copy instance data and render
  SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(app->device);
  if (cmdbuf == NULL) {
    SDL_Log("Error: SDL_AcquireGPUCommandBuffer(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmdbuf);
  {
    // Upload dynamic data
    UploadQuadGroupFrame(app->quad_group, app->device, copy_pass);
  }
  SDL_EndGPUCopyPass(copy_pass);
  SDL_SubmitGPUCommandBuffer(cmdbuf);

  cmdbuf = SDL_AcquireGPUCommandBuffer(app->device);
  if (cmdbuf == NULL) {
    SDL_Log("Error: SDL_AcquireGPUCommandBuffer(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUTexture* swapchain_texture = NULL;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, app->window, &swapchain_texture, NULL, NULL)) {
    SDL_Log("Warning: could not acquire GPU swapchain texture");
  }

  if (swapchain_texture != NULL) {
    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = (SDL_FColor){0.2f, 0.2f, 0.5f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
    {
      SDL_SetGPUViewport(render_pass, &app->viewport);

      // Bind flat color pipeline
      SDL_BindGPUGraphicsPipeline(render_pass, app->flat_color_pipeline);
      {
        // Render the quad using the bound pipeline
        RenderQuadGroup(app->quad_group, cmdbuf, render_pass);
      }
      SDL_BindGPUGraphicsPipeline(render_pass, NULL);
    }
    SDL_EndGPURenderPass(render_pass);
  }

  SDL_SubmitGPUCommandBuffer(cmdbuf);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  ExampleApp* app = (ExampleApp*)appstate;
  SDL_assert(app != NULL);

  switch (event->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      if (event->window.windowID == SDL_GetWindowID(app->window)) {
        return SDL_APP_SUCCESS;
      }
      break;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  ExampleApp* app = (ExampleApp*)appstate;
  if (app == NULL) {
    return;
  }

  if (app->quad_group != NULL) {
    DestroyQuadGroup(app->quad_group, app->device);
  }

  if (app->flat_color_pipeline != NULL) {
    SDL_ReleaseGPUGraphicsPipeline(app->device, app->flat_color_pipeline);
  }

  if (app->device != NULL) {
    SDL_DestroyGPUDevice(app->device);
  }

  if (app->window != NULL) {
    SDL_DestroyWindow(app->window);
  }
  SDL_Log("Info: Terminated with result: %d", result);
}
