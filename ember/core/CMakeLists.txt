add_compile_definitions(
    EMBER_CORE_LOG="ember.core"
)

add_library(ember-core
    STATIC
    src/SceneManager.cpp
)
target_link_libraries(ember-core
    PUBLIC
    ember-ecs
    ember-graphics
    PRIVATE
    ember-util
)
target_include_directories(ember-core
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

if(EMBER_TESTS)
    add_executable(ember-core.tests.unit
        tests/test_scene_manager.cpp
    )
    target_include_directories(
        ember-core.tests.unit
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-core.tests.unit PRIVATE ember-core Catch2::Catch2WithMain)
    add_test(NAME ember-core.tests.unit COMMAND $<TARGET_FILE:ember-core.tests.unit>)
endif()
