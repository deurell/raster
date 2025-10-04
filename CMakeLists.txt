cmake_minimum_required(VERSION 3.16)
project(raster C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Check if Emscripten is being used
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(EMSCRIPTEN_BUILD TRUE)
    message(STATUS "Building for Emscripten")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --embed-file ${CMAKE_BINARY_DIR}/assets@/assets")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --embed-file ${CMAKE_BINARY_DIR}/assets@/assets")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s SINGLE_FILE=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    add_compile_definitions(__EMSCRIPTEN__)
endif()

# Add external dependencies
if(NOT EMSCRIPTEN_BUILD)
    # Configure GLFW - not needed for Emscripten as it's bundled
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/glfw)
endif()

# Configure GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Add the raster library
add_library(raster STATIC
    src/raster/impl/raster_app.c
    src/raster/impl/raster_gfx_common.c
    src/raster/impl/raster_gfx_sprite.c
    src/raster/impl/raster_gfx_text.c
    src/raster/impl/raster_input.c
    src/raster/impl/raster_log.c
    src/raster/impl/raster_sfx.c
    src/raster/impl/raster_transform.c
)

target_include_directories(raster 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

if(EMSCRIPTEN_BUILD)
    target_link_libraries(raster PUBLIC glad)
else()
    target_link_libraries(raster PUBLIC glad glfw ${CMAKE_DL_LIBS})
    if(UNIX AND NOT APPLE)
        target_link_libraries(raster PUBLIC GL)
    endif()
endif()

# Create the assets directory in the build folder
add_custom_target(create_assets_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/assets
    COMMENT "Creating assets directory"
)

# Copy engine assets to build directory
add_custom_target(copy_engine_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying engine assets to build directory"
    DEPENDS create_assets_dir
)

# Define a variable to be used in subdirectories
set(RASTER_ASSETS_DIR ${CMAKE_BINARY_DIR}/assets)

# Add examples
add_subdirectory(examples/hello_world)
