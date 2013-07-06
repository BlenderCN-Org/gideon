cmake_minimum_required(VERSION 2.8)
project(Gideon)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(BLENDER_ADDON_ROOT "" CACHE PATH "Location of Blender plugins.")

add_subdirectory(src)
