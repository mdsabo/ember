cmake_minimum_required(VERSION 4.0)
project(ember VERSION 0.0.1)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(ember)
# add_subdirectory(examples)
add_subdirectory(third_party/Catch2)