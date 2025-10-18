add_library(ember-util
    STATIC
    src/ArgParser.cpp
    src/Filesystem.cpp
    src/Log.cpp
)
target_include_directories(ember-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

if(EMBER_TESTS)
    add_executable(ember-util.test.unit
        tests/test_arg_parser.cpp
        tests/test_log.cpp
    )
    target_include_directories(ember-util.test.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ember-util.test.unit PRIVATE ember-util Catch2::Catch2WithMain)
    add_test(NAME ember-util.test.unit COMMAND $<TARGET_FILE:ember-util.test.unit>)
endif()
