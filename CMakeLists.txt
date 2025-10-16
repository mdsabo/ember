cmake_minimum_required(VERSION 4.0)
project(ember VERSION 0.0.1)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(ember)
# add_subdirectory(examples)
add_subdirectory(third_party/Catch2)

# SPIRV-Reflect doesn't advertise any packages so build one ourselves (which is how they say to do it)
# https://github.com/KhronosGroup/SPIRV-Reflect/blob/main/README.md#integration
add_library(SPIRV-Reflect third_party/SPIRV-Reflect/spirv_reflect.c)
target_include_directories(SPIRV-Reflect PUBLIC third_party/SPIRV-Reflect)