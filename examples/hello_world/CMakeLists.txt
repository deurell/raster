cmake_minimum_required(VERSION 3.16)

add_executable(hello_world main.c)
target_link_libraries(hello_world PRIVATE raster)

# Emscripten-specific settings for this example
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set_target_properties(
        hello_world
        PROPERTIES
        SUFFIX ".html"
    )
    # No additional preload options needed - handled at the root level
endif()

# Add dependency on engine assets
add_dependencies(hello_world copy_engine_assets)

# Copy hello_world's assets to the shared assets directory if the directory exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    add_custom_command(
        TARGET hello_world PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            ${RASTER_ASSETS_DIR}
        COMMENT "Copying hello_world assets to the shared assets directory"
    )
endif()

# For non-Emscripten builds, also copy the combined assets to the example's output directory
if(NOT EMSCRIPTEN_BUILD)
    add_custom_command(
        TARGET hello_world POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${RASTER_ASSETS_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/assets
        COMMENT "Copying combined assets to hello_world executable directory"
    )
endif()
