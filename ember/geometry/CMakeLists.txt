add_library(ember-geometry
    STATIC
    src/IntersectionTests.cpp
)
target_include_directories(ember-geometry
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-geometry PUBLIC eigen)

if(EMBER_TESTS)
    add_executable(ember-geometry.test.unit tests/test_ray.cpp)
    target_include_directories(ember-geometry.test.unit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(ember-geometry.test.unit PRIVATE ember-geometry Catch2::Catch2WithMain)
    add_test(NAME ember-geometry.test.unit COMMAND $<TARGET_FILE:ember-geometry.test.unit>)
endif()

if(EMBER_EXAMPLES)
    # add_executable(ember-geometry.example.compute-raytracer examples/compute_raytracer.cpp)
    # target_include_directories(ember-geometry.example.compute-raytracer
    #     PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}
    #     ${CMAKE_SOURCE_DIR}
    # )
    # target_link_libraries(ember-geometry.example.compute-raytracer PRIVATE ember-geometry eigen stb)
endif()
