cmake_minimum_required(VERSION 3.16)

add_executable(hello_world main.c)
target_link_libraries(hello_world PRIVATE raster)
