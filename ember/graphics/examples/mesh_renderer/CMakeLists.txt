if(EMBER_EXAMPLES)
    add_executable(ember-graphics.examples.mesh-renderer main.cpp)
    target_include_directories(ember-graphics.examples.mesh-renderer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/ember/graphics
    )
    target_link_libraries(ember-graphics.examples.mesh-renderer PRIVATE ember-graphics assimp SDL3::SDL3)
    if(WIN32)
        add_custom_command(
            TARGET ember-graphics.examples.mesh-renderer
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:ember-graphics.examples.mesh-renderer>
                $<TARGET_FILE_DIR:ember-graphics.examples.mesh-renderer>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()