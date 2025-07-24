cmake_minimum_required(VERSION 3.28)
project(mecs LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mecs
    STATIC
    src/mecs/registry.cc
    src/mecs/world.cc
    src/mecs/iterator.cc
    src/mecs/private.cc
    src/mecs/collections.cc

    include/mecs/defines.h
    include/mecs/base.h
    include/mecs/mecs.h
    include/mecs/registry.h
    include/mecs/world.h
    include/mecs/iterator.h

    src/mecs/private.h
    src/mecs/collections.h
)

target_include_directories(mecs PUBLIC include)

set_target_properties(mecs PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

add_executable(mecs_test src/main.cc)
target_link_libraries(mecs_test PRIVATE mecs)
set_target_properties(mecs_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)