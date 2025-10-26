
add_executable(ember.tools.meshviewer
    main.cpp
    MeshViewer.cpp
)
target_compile_definitions(ember.tools.meshviewer
    PRIVATE
    VULKAN_HPP_NO_CONSTRUCTORS
    MESHVIEWER_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    MESHVIEWER_LOG="meshviewer"
)
target_link_libraries(ember.tools.meshviewer
    PRIVATE
    ember
    assimp
    glm
    SDL3::SDL3
)
if(WIN32)
    add_custom_command(
        TARGET ember.tools.meshviewer
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ember.tools.meshviewer>
            $<TARGET_FILE_DIR:ember.tools.meshviewer>
        COMMAND_EXPAND_LISTS
    )
endif()