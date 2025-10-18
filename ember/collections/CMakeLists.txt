add_library(ember-collections STATIC src/DynamicBitset.cpp)
target_include_directories(ember-collections PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

if(EMBER_TESTS)
    add_executable(ember-collections.test.unit tests/test_dynamic_bitset.cpp)
    target_include_directories(ember-collections.test.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ember-collections.test.unit PRIVATE ember-collections Catch2::Catch2WithMain)
    add_test(NAME ember-collections.test.unit COMMAND $<TARGET_FILE:ember-collections.test.unit>)
endif()