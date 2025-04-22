cmake_minimum_required(VERSION 3.16)
project(raster C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add external dependencies
add_subdirectory(external/glfw)

# Configure GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Add the raster library
add_library(raster STATIC
    src/raster/impl/raster_app.c
    src/raster/impl/raster_gfx.c
    src/raster/impl/raster_input.c
    src/raster/impl/raster_log.c
    src/raster/impl/stb_image_impl.c
)

target_include_directories(raster 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
)
target_link_libraries(raster PUBLIC glad glfw)

# Copy assets to build directory
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
)

# Define a variable to be used in subdirectories
set(RASTER_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Add examples
add_subdirectory(examples/hello_world)
