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
    # add_executable(ember-graphics.test.unit tests/test_ray.cpp)
    # target_include_directories(ember-graphics.test.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    # target_link_libraries(ember-graphics.test.unit PRIVATE ember-graphics Catch2::Catch2WithMain)
    # add_test(NAME ember-graphics.test.unit COMMAND $<TARGET_FILE:ember-graphics.test.unit>)
endif()

if(EMBER_EXAMPLES)
    add_executable(ember-graphics.example.sdl-playground examples/sdl_playground.cpp)
    target_include_directories(ember-graphics.example.sdl-playground
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-graphics.example.sdl-playground PRIVATE ember-graphics SDL3::SDL3)
endif()
