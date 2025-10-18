add_library(ember-collections STATIC src/DynamicBitset.cpp)
target_include_directories(ember-collections PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

if(EMBER_TESTS)
    add_executable(test-ember-collections tests/test_dynamic_bitset.cpp)
    target_include_directories(test-ember-collections PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test-ember-collections PRIVATE ember-collections Catch2::Catch2WithMain)
    add_test(NAME test-ember-collections COMMAND $<TARGET_FILE:test-ember-collections>)
endif()