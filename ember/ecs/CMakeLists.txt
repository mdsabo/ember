add_library(ember-ecs STATIC src/EntitySet.cpp)
target_include_directories(ember-ecs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ember-ecs PRIVATE ember-collections)

add_executable(test-ember-ecs 
    tests/test_entity_set.cpp
    tests/test_storage.cpp
)
target_include_directories(test-ember-ecs 
    PRIVATE  
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test-ember-ecs PRIVATE ember-ecs Catch2::Catch2WithMain)
add_test(NAME test-ember-ecs COMMAND $<TARGET_FILE:test-ember-ecs> --skip-benchmarks)