add_library(ember-ecs
    STATIC
    src/EntitySet.cpp
    src/World.cpp
)
target_include_directories(ember-ecs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ember-ecs PRIVATE ember-collections)

if(EMBER_TESTS)
    add_executable(ember-ecs.test.unit
        tests/test_entity_set.cpp
        tests/test_storage.cpp
        tests/test_world.cpp
    )
    target_include_directories(ember-ecs.test.unit
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-ecs.test.unit PRIVATE ember-ecs Catch2::Catch2WithMain)
    add_test(NAME ember-ecs.test.unit COMMAND $<TARGET_FILE:ember-ecs.test.unit> --skip-benchmarks)
endif()