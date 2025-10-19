add_library(ember-graphics
    STATIC
    src/Window.cpp
)
target_include_directories(ember-graphics
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-graphics PUBLIC SDL3::SDL3)

if(EMBER_TESTS)
    # add_executable(ember-graphics.tests.unit tests/test_ray.cpp)
    # target_include_directories(ember-graphics.tests.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    # target_link_libraries(ember-graphics.tests.unit PRIVATE ember-graphics Catch2::Catch2WithMain)
    # add_test(NAME ember-graphics.tests.unit COMMAND $<TARGET_FILE:ember-graphics.tests.unit>)
endif()

if(EMBER_EXAMPLES)
    add_executable(ember-graphics.examples.sdl-playground examples/sdl_playground.cpp)
    target_include_directories(ember-graphics.examples.sdl-playground
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-graphics.examples.sdl-playground PRIVATE ember-graphics SDL3::SDL3)
    add_custom_command(
        TARGET ember-graphics.examples.sdl-playground
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ember-graphics.examples.sdl-playground>
            $<TARGET_FILE_DIR:ember-graphics.examples.sdl-playground>
    )
endif()
