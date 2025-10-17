find_package(Vulkan REQUIRED shaderc_combined)

add_library(ember-gpu
    STATIC
    src/CommandRecorder.cpp
    src/GraphicsDevice.cpp
    src/Renderer.cpp
    src/ShaderReflection.cpp
    src/SPIRV.cpp
)
target_include_directories(ember-gpu
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-gpu ember-util Vulkan::Vulkan Vulkan::shaderc_combined spirv-reflect-static)

# add_executable(test-ember-gpu tests/test_dynamic_bitset.cpp)
# target_include_directories(test-ember-gpu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(test-ember-gpu PRIVATE ember-gpu Catch2::Catch2WithMain)
# add_test(NAME test-ember-gpu COMMAND $<TARGET_FILE:test-ember-gpu>)

add_executable(ember-gpu-compute-squares examples/compute_squares.cpp)
target_include_directories(ember-gpu-compute-squares PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ember-gpu-compute-squares PRIVATE ember-gpu ember-util)
