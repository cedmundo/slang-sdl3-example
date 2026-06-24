cmake_minimum_required(VERSION 3.22)
project(slang-sdl3-example
  VERSION "1.0.0"
  DESCRIPTION "Example of slang with SDL3"
  LANGUAGES C)

# CMake options
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Vendored dependencies
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

# Main target
add_executable(slang-sdl3-example)
target_sources(slang-sdl3-example PRIVATE shader.c quad.c main.c)
target_link_libraries(slang-sdl3-example PRIVATE SDL3)
