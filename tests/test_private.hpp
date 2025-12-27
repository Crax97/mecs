#pragma once

#include <catch_amalgamated.hpp>

#include "mecs/base.h"
#include "mecs/mecs.h"
#include "private.h"

#define MECS_COMPONENT(world, ent, C) \
    *(C*)mecsWorldAddComponent(world, ent, Component_##C)

#ifdef MECS_TESTS_LEAK_DETECTION

void* debugMemalloc(void* userData, MecsSize size, MecsSize align);
void* debugMemRealloc(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize);
void debugMemFree(void* userData, void* ptr);

void debugAllocatorCheck();

constexpr MecsAllocator kDebugAllocator = {
    .memAlloc = debugMemalloc,
    .memRealloc = debugMemRealloc,
    .memFree = debugMemFree,
    .userData = nullptr
};
#else
constexpr MecsAllocator kDebugAllocator = kDefaultAllocator;
#endif
