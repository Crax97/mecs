#pragma once

#include "defines.h"
#include <cstdint>
#include <stddef.h>

#ifndef MECS_ASSERT
#include <assert.h>
#define MECS_ASSERT(condition) assert((condition))
#endif

#define MECS_COMPONENTINFO(T) { .name = #T, .size = sizeof(T), .align = _Alignof(T) }

#define MECS_INVALID ~0U

MECS_EXTERNCPP()
/// NOLINTBEGIN

typedef uint8_t MecsU8;
typedef uint32_t MecsU32;
typedef size_t MecsSize;

typedef MecsU32 MecsEntityID;
typedef MecsU32 MecsComponentID;
typedef MecsU32 MecsComponentInstanceID;

typedef void* (*PFNMecsMalloc)(void* userData, MecsSize size, MecsSize align);
typedef void* (*PFNMecsRealloc)(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize);
typedef void (*PFNMecsFree)(void* userData, void* ptr);

typedef void (*PFNMecsComponentInit)(void* mem);
typedef void (*PFNMecsComponentCopy)(void* source, void* dest,
    MecsSize componentSize);
typedef void (*PFNMecsComponentDestroy)(void* mem);

typedef struct MECS_API MecsRegistry_t MecsRegistry;
typedef struct MECS_API MecsWorld_t MecsWorld;
typedef struct MECS_API MecsWorldIterator_t MecsIterator;

typedef struct MecsAllocator {
    // If null, will use an internal malloc function
    PFNMecsMalloc memAlloc;

    // If null, will use an internal realloc function
    PFNMecsRealloc memRealloc;

    // If null, will use an internal free function
    PFNMecsFree memFree;

    void* userData;
} MecsAllocator;

typedef struct MecsRegistryCreateInfo {
    MecsAllocator memAllocator;
} MecsRegistryCreateInfo;

typedef struct MecsWorldCreateInfo {
    // Must not be null
    MecsRegistry* registry;

    MecsAllocator memAllocator;
} MecsWorldCreateInfo;

typedef struct ComponentInfo {
    // Must not be null and unique among all components registered
    const char* name;

    // Must be greater than zero
    MecsSize size;

    // Must be greater than zero
    MecsSize align;

    // Can be null, called when this component is added to an entity
    // to initialize the component
    PFNMecsComponentInit init;

    // Can be null, called when this component is copied in memory
    PFNMecsComponentCopy copy;

    // Can be null, called when this component is removed from an entity
    // to deinitialize the component
    PFNMecsComponentDestroy destroy;
} ComponentInfo;

typedef struct MecsEntityInfo {
    // Can be null
    const char* name;

} MecsEntityInfo;

/// NOLINTEND
MECS_ENDEXTERNCPP()