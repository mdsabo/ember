add_library(ember-geometry
    STATIC
    src/IntersectionTests.cpp
)
target_include_directories(ember-geometry
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-geometry PUBLIC glm)

if(EMBER_TESTS)
    add_executable(ember-geometry.tests.unit tests/test_ray.cpp)
    target_include_directories(ember-geometry.tests.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ember-geometry.tests.unit PRIVATE ember-geometry Catch2::Catch2WithMain)
    add_test(NAME ember-geometry.tests.unit COMMAND $<TARGET_FILE:ember-geometry.tests.unit>)
endif()

if(EMBER_EXAMPLES)
    # add_executable(ember-geometry.examples.compute-raytracer examples/compute_raytracer.cpp)
    # target_include_directories(ember-geometry.examples.compute-raytracer
    #     PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}
    #     ${CMAKE_SOURCE_DIR}
    # )
    # target_link_libraries(ember-geometry.examples.compute-raytracer PRIVATE ember-geometry glm stb)
endif()
