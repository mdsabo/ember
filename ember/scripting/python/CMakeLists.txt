find_package(Python COMPONENTS Interpreter Development REQUIRED)

# add_library(ember-scripting-python
#     STATIC

# )
# target_include_directories(
#     ember-scripting-python
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_SOURCE_DIR}
# )

if(EMBER_TESTS)
    add_executable(ember-scripting-python.tests.unit
        tests/test_python.cpp
    )
    target_include_directories(ember-scripting-python.tests.unit
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(ember-scripting-python.tests.unit
        PRIVATE Catch2::Catch2WithMain Python::Python
    )
    add_test(
        NAME ember-scripting-python.tests.unit
        COMMAND $<TARGET_FILE:ember-scripting-python.examples.playground>
    )
endif()

if (EMBER_EXAMPLES)
    add_executable(ember-scripting-python.examples.playground
        examples/playground.cpp
    )
    target_include_directories(ember-scripting-python.examples.playground
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(ember-scripting-python.examples.playground
        PRIVATE Python::Python
    )
endif()
