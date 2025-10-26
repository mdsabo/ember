add_executable(ember.examples.e1
    main.cpp
)
target_link_libraries(ember.examples.e1
    ember
)
if(WIN32)
    add_custom_command(
        TARGET ember.examples.e1
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ember.examples.e1>
            $<TARGET_FILE_DIR:ember.examples.e1>
        COMMAND_EXPAND_LISTS
    )
endif()