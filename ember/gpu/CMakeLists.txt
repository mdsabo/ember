find_package(Vulkan REQUIRED shaderc_combined)
add_compile_definitions(EMBER_GPU_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

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

if(EMBER_TESTS)
    add_executable(ember-gpu.test.compute-squares tests/compute_squares.cpp)
    target_include_directories(ember-gpu.test.compute-squares
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-gpu.test.compute-squares PRIVATE ember-gpu ember-util)
    add_test(NAME ember-gpu.test.compute-squares COMMAND $<TARGET_FILE:ember-gpu.test.compute-squares>)

# add_executable(ember-gpu.test tests/test_dynamic_bitset.cpp)
# target_include_directories(ember-gpu.test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(ember-gpu.test PRIVATE ember-gpu Catch2::Catch2WithMain)
# add_test(NAME ember-gpu.test COMMAND $<TARGET_FILE:ember-gpu.test>)
endif()

if(EMBER_EXAMPLES)
    add_executable(ember-gpu.example.compute-raytracer examples/compute_raytracer.cpp)
    target_include_directories(ember-gpu.example.compute-raytracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-gpu.example.compute-raytracer PRIVATE ember-gpu eigen stb)
endif()
