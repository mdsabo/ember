if(EMBER_EXAMPLES)
    add_executable(ember-graphics.examples.sdl-window main.cpp)
    target_include_directories(ember-graphics.examples.sdl-window
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/ember/graphics
    )
    target_link_libraries(ember-graphics.examples.sdl-window PRIVATE ember-graphics SDL3::SDL3)
    if(WIN32)
        add_custom_command(
            TARGET ember-graphics.examples.sdl-window
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:ember-graphics.examples.sdl-window>
                $<TARGET_FILE_DIR:ember-graphics.examples.sdl-window>
        )
    endif()
endif()