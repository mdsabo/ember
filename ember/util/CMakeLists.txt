add_library(ember-util
    STATIC
    src/ArgParser.cpp
    src/Filesystem.cpp
    src/Log.cpp
)
target_include_directories(ember-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

if(EMBER_TESTS)
    add_executable(ember-util.tests.unit
        tests/test_allocators.cpp
        tests/test_arg_parser.cpp
        tests/test_log.cpp
    )
    target_include_directories(ember-util.tests.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ember-util.tests.unit PRIVATE ember-util Catch2::Catch2WithMain)
    add_test(NAME ember-util.tests.unit COMMAND $<TARGET_FILE:ember-util.tests.unit>)
endif()
