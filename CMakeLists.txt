cmake_minimum_required(VERSION 4.0)
project(ember VERSION 0.0.1)
include(CTest)

option(EMBER_TESTS "Build ember tests" ON)
option(EMBER_EXAMPLES "Build ember example programs" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


add_subdirectory(ember)

if(EMBER_EXAMPLES)
# add_subdirectory(examples)
endif()

add_subdirectory(third_party/assimp EXCLUDE_FROM_ALL)
add_subdirectory(third_party/Catch2 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/eigen EXCLUDE_FROM_ALL)

set(SDL_TEST_LIBRARY OFF)
add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
add_subdirectory(third_party/SPIRV-Reflect)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> # ???
)