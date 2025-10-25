add_compile_definitions(EMBER_ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")

add_subdirectory(collections)
add_subdirectory(ecs)
add_subdirectory(geometry)
add_subdirectory(gpu)
add_subdirectory(graphics)
add_subdirectory(util)

add_library(ember INTERFACE)
target_include_directories(ember INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ember INTERFACE
    ember-collections
    ember-ecs
    ember-geometry
    ember-gpu
    ember-graphics
    ember-util
)