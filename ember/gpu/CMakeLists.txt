find_package(Vulkan REQUIRED shaderc_combined)

add_compile_definitions(
    VULKAN_HPP_NO_CONSTRUCTORS
    EMBER_GPU_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    EMBER_GPU_LOG="ember.gpu"
)

add_library(ember-gpu
    STATIC
    src/CommandRecorder.cpp
    src/GPUDevice.cpp
    src/GPUInterface.cpp
    src/ShaderReflection.cpp
    src/SPIRV.cpp
    src/VulkanInstance.cpp
)
target_include_directories(ember-gpu
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-gpu
    PUBLIC
    ember-util
    SDL3::SDL3
    spirv-reflect-static
    Vulkan::Vulkan

    PRIVATE
    ember-core
    Vulkan::shaderc_combined
)

if(EMBER_TESTS)
    add_executable(ember-gpu.tests.compute-squares tests/compute_squares.cpp)
    target_include_directories(ember-gpu.tests.compute-squares
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-gpu.tests.compute-squares PRIVATE ember-core ember-gpu)
    if(WIN32)
        add_custom_command(
            TARGET ember-gpu.tests.compute-squares
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:ember-gpu.tests.compute-squares>
                $<TARGET_FILE_DIR:ember-gpu.tests.compute-squares>
            COMMAND_EXPAND_LISTS
        )
    endif()
    add_test(NAME ember-gpu.tests.compute-squares COMMAND $<TARGET_FILE:ember-gpu.tests.compute-squares>)

# add_executable(ember-gpu.test tests/test_dynamic_bitset.cpp)
# target_include_directories(ember-gpu.test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(ember-gpu.test PRIVATE ember-gpu Catch2::Catch2WithMain)
# add_test(NAME ember-gpu.test COMMAND $<TARGET_FILE:ember-gpu.test>)
endif()

if(EMBER_EXAMPLES)
    add_executable(ember-gpu.examples.compute-raytracer examples/compute_raytracer.cpp)
    target_include_directories(ember-gpu.examples.compute-raytracer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(ember-gpu.examples.compute-raytracer PRIVATE -Wno-deprecated-declarations)
    endif()
    target_link_libraries(ember-gpu.examples.compute-raytracer PRIVATE ember-gpu glm stb)
    if(WIN32)
        add_custom_command(
            TARGET ember-gpu.examples.compute-raytracer
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:ember-gpu.examples.compute-raytracer>
                $<TARGET_FILE_DIR:ember-gpu.examples.compute-raytracer>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()
