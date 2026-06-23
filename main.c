#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_video.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define WINDOW_TITLE "Slang + SDLGPU Example"
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 500

typedef struct {
  SDL_Window *window;
  SDL_GPUDevice *device;
  SDL_GPUViewport viewport;
} ExampleApp;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Error: SDL_Init(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  ExampleApp *app = SDL_malloc(sizeof(ExampleApp));
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
  app->viewport =
      (SDL_GPUViewport){.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};

  app->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (app->device == NULL) {
    SDL_Log("Error: SDL_CreateGPUDevice(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(app->device, app->window)) {
    SDL_Log("Error: SDL_ClaimWindowForGPUDevice(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  ExampleApp *app = (ExampleApp *)appstate;
  SDL_assert(app != NULL);

  SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(app->device);
  if (cmdbuf == NULL) {
    SDL_Log("Error: SDL_AcquireGPUCommandBuffer(): %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUTexture *swapchain_texture = NULL;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, app->window,
                                             &swapchain_texture, NULL, NULL)) {
    SDL_Log("Warning: could not acquire GPU swapchain texture");
  }

  if (swapchain_texture != NULL) {
    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = (SDL_FColor){0.2f, 0.2f, 0.5f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *render_pass =
        SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, NULL);
    {
      SDL_SetGPUViewport(render_pass, &app->viewport);

      // Render anything else ...
    }
    SDL_EndGPURenderPass(render_pass);
  }

  SDL_SubmitGPUCommandBuffer(cmdbuf);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  ExampleApp *app = (ExampleApp *)appstate;
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

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  ExampleApp *app = (ExampleApp *)appstate;
  if (app == NULL) {
    return;
  }

  SDL_DestroyGPUDevice(app->device);
  SDL_DestroyWindow(app->window);
  SDL_Log("Info: Terminated with result: %d", result);
}
