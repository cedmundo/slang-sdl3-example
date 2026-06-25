cmake_minimum_required(VERSION 3.22)
project(slang-sdl3-example
  VERSION "1.0.0"
  DESCRIPTION "Example of slang with SDL3"
  LANGUAGES C)

# CMake options
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

# Vendored dependencies
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
FetchContent_Declare(
  SDL3_image
  GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
  GIT_TAG "release-3.4.4"
)
FetchContent_MakeAvailable(SDL3_image)

# Custom target for shaders
add_subdirectory(assets)

# Main target
add_executable(slang-sdl3-example)
target_sources(slang-sdl3-example PRIVATE shader.c quad.c texture.c main.c)
target_link_libraries(slang-sdl3-example PRIVATE SDL3 SDL3_image-shared)
add_dependencies(slang-sdl3-example slang-sdl3-example-vs slang-sdl3-example-fs slang-sdl3-example-texture)
