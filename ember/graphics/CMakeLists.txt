add_compile_definitions(
    VULKAN_HPP_NO_CONSTRUCTORS
    EMBER_GRAPHICS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    EMBER_GRAPHICS_LOG="ember.graphics"
)

add_library(ember-graphics
    STATIC
    src/MeshRenderSystem.cpp
    src/Renderer.cpp
    src/Renderpass.cpp
    src/Window.cpp
)
target_include_directories(ember-graphics
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-graphics
    PUBLIC
    assimp
    ember-ecs
    ember-gpu
    glm
)

if(EMBER_TESTS)
    # add_executable(ember-graphics.tests.unit tests/test_ray.cpp)
    # target_include_directories(ember-graphics.tests.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    # target_link_libraries(ember-graphics.tests.unit PRIVATE ember-graphics Catch2::Catch2WithMain)
    # add_test(NAME ember-graphics.tests.unit COMMAND $<TARGET_FILE:ember-graphics.tests.unit>)
endif()

add_subdirectory(examples/sdl_window)
