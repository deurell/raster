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
endif()

# Add dependency on copy_assets
add_dependencies(hello_world copy_assets)

# Also copy assets to this specific output directory
add_custom_command(
    TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RASTER_ASSETS_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Copying assets to hello_world executable directory"
)
