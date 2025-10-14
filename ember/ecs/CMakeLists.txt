add_library(ember-ecs STATIC lib.cpp)
target_link_libraries(ember-ecs PRIVATE ember-collections)