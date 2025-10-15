add_subdirectory(collections)
add_subdirectory(ecs)
add_subdirectory(gpu)
add_subdirectory(util)

add_library(ember INTERFACE)
target_include_directories(ember INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ember INTERFACE
    ember-collections
    ember-ecs
)