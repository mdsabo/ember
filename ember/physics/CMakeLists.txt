add_library(ember-physics
    STATIC
    src/ColliderComponent.cpp
    src/CollisionSystem.cpp
    src/RigidBodyComponent.cpp
    src/RigidBodySystem.cpp
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
    PRIVATE
    ember-geometry
    ember-util
)

if(EMBER_TESTS)
    add_executable(ember-physics.tests.unit
        tests/test_rigid_body_component.cpp
    )
    target_include_directories(ember-physics.tests.unit
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(ember-physics.tests.unit PRIVATE ember-physics Catch2::Catch2WithMain)
    add_test(NAME ember-physics.tests.unit COMMAND $<TARGET_FILE:ember-physics.tests.unit>)
endif()
