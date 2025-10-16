add_library(ember-util 
    STATIC 
    src/ArgParser.cpp
    src/Filesystem.cpp
)
target_include_directories(ember-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

add_executable(test-ember-util tests/test_arg_parser.cpp)
target_include_directories(test-ember-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-ember-util PRIVATE ember-util Catch2::Catch2WithMain)
add_test(NAME test-ember-util COMMAND $<TARGET_FILE:test-ember-util>)