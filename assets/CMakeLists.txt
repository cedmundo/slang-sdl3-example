add_custom_target(slang-sdl3-example-vs
    COMMAND slangc
      -target spirv
      -profile spirv_1_3
      -o ${CMAKE_BINARY_DIR}/flat-color.vs.spirv
      -entry main
      ${CMAKE_SOURCE_DIR}/assets/shaders/flat-color.vs.slang
    DEPENDS ${CMAKE_SOURCE_DIR}/assets/shaders/flat-color.vs.slang
    BYPRODUCTS ${CMAKE_BINARY_DIR}/flat-color.vs.spirv)

add_custom_target(slang-sdl3-example-fs
    COMMAND slangc
      -target spirv
      -profile spirv_1_3
      -o ${CMAKE_BINARY_DIR}/flat-color.fs.spirv
      -entry main
      ${CMAKE_SOURCE_DIR}/assets/shaders/flat-color.fs.slang
    DEPENDS ${CMAKE_SOURCE_DIR}/assets/shaders/flat-color.fs.slang
    BYPRODUCTS ${CMAKE_BINARY_DIR}/flat-color.fs.spirv)

add_custom_target(slang-sdl3-example-cs
    COMMAND slangc
      -target spirv
      -profile spirv_1_3
      -o ${CMAKE_BINARY_DIR}/quad-group.spirv
      -entry main
      ${CMAKE_SOURCE_DIR}/assets/shaders/quad-group.slang
    DEPENDS ${CMAKE_SOURCE_DIR}/assets/shaders/quad-group.slang
    BYPRODUCTS ${CMAKE_BINARY_DIR}/quad-group.spirv)

add_custom_target(slang-sdl3-example-texture
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/assets/textures/quad.png
      ${CMAKE_BINARY_DIR}/quad.png
    DEPENDS ${CMAKE_SOURCE_DIR}/assets/textures/quad.png
    BYPRODUCTS ${CMAKE_BINARY_DIR}/quad.png)
