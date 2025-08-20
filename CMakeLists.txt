cmake_minimum_required(VERSION 3.28)
project(mecs LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MECS_COMPILE_TESTS "Compile tests (requires git submodules)" ON)
option(MECS_TESTS_LEAK_DETECTION "Use leak detection allocator in tests" OFF)

add_library(mecs
    STATIC
    src/mecs/registry.cc
    src/mecs/world.cc
    src/mecs/iterator.cc
    src/mecs/private.cc
    src/mecs/collections.cc
    src/mecs/archetype.cc
    src/mecs/collections.natvis

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


add_library(mecshpp
    STATIC
    src/mecshpp/mecs.cc

    include/mecshpp/mecs.hpp
)

target_include_directories(mecshpp PUBLIC include)
set_target_properties(mecshpp PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
target_link_libraries(mecshpp PUBLIC mecs)

if(MECS_COMPILE_TESTS)
    add_subdirectory(thirdparty/catch2)

    add_executable(mecs_tests
        tests/base.cc
        tests/collections.cc
        tests/prefabs.cc
        tests/test_private.cc
        tests/samples.cc
        tests/reflection.cc
    )

    target_include_directories(mecs_tests PRIVATE src/mecs)
    target_link_libraries(mecs_tests PRIVATE mecshpp MecsCatch2)
    set_target_properties(mecs_tests PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
    if(MECS_TESTS_LEAK_DETECTION)
        message(STATUS "Enabling leak detection in tests")
        target_compile_definitions(mecs_tests PRIVATE -DMECS_TESTS_LEAK_DETECTION)
    endif()
endif()
