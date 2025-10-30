add_library(ember-physics
    STATIC
    src/ParticleSystem.cpp
    src/SpringSystem.cpp
)
target_include_directories(ember-physics
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ember-physics
    PUBLIC
    ember-ecs
    glm
)

if(EMBER_TESTS)
    add_executable(ember-physics.tests.unit
        tests/test_particle_system.cpp
        tests/test_spring_system.cpp
    )
    target_include_directories(ember-physics.tests.unit
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-physics.tests.unit PRIVATE ember-physics Catch2::Catch2WithMain)
    add_test(NAME ember-physics.tests.unit COMMAND $<TARGET_FILE:ember-physics.tests.unit>)
endif()
