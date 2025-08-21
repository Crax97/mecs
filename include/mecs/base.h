#pragma once

#include "defines.h"
#include <cstdint>
#include <stddef.h>

#ifndef MECS_ASSERT
#include <assert.h>
#define MECS_ASSERT(condition) assert((condition))
#endif

#define MECS_INVALID ~0U

#ifdef __cplusplus
#define MECS_ALIGN_OF alignof
#else
#define MECS_ALIGN_OF _Alignof
#endif
#define MECS_COMPONENTINFO(T) { .name = #T, .typeID = __COUNTER__, .size = sizeof(T), .align = MECS_ALIGN_OF(T) }

#define MECS_REGISTER_COMPONENT(reg, T)                          \
    static MecsComponentID Component_##T = MECS_INVALID;         \
    do {                                                         \
        ComponentInfo info = MECS_COMPONENTINFO(T);              \
        Component_##T = mecsRegistryAddRegistration(reg, &info); \
    } while (0)

MECS_EXTERNCPP()
/// NOLINTBEGIN

typedef uint8_t MecsU8;
typedef uint32_t MecsU32;
typedef uint64_t MecsTypeID;
typedef size_t MecsSize;

typedef MecsU32 MecsEntityID;
typedef MecsU32 MecsPrefabID;
typedef MecsU32 MecsComponentID;

typedef void* (*PFNMecsMalloc)(void* userData, MecsSize size, MecsSize align);
typedef void* (*PFNMecsRealloc)(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize);
typedef void (*PFNMecsFree)(void* userData, void* ptr);

typedef void (*PFNMecsComponentInit)(void* mem);
typedef void (*PFNMecsComponentCopy)(const void* source, void* dest,
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
    MecsAllocator memAllocator;
} MecsWorldCreateInfo;

typedef struct MecsComponentMember {
    // Must be a valid ID
    MecsTypeID typeID;

    // Offset of the member in the component
    MecsSize offset;

    // Must not be null
    const char* name;
} MecsComponentMember;

typedef struct ComponentInfo {
    // Must not be null
    const char* name;

    // Must be unique among all components in the registry: if a component has the same typeID and name of another registered type,
    // the older type will be replaced
    MecsTypeID typeID;

    // Must be greater than zero
    MecsSize size;

    // Must be greater than zero
    MecsSize align;

    // Number of child members
    MecsSize memberCount;

    // Can be null only when memberCount is 0: in that case, the field is ignored.
    // This field must live for as long as the parent registry
    const MecsComponentMember* members;

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

typedef struct MecsPrefabInfo {
    // Can be null
    const char* name;

} MecsPrefabInfo;

enum MecsIteratorFilter {
    Access, // Retrieves a pointer to the component
    With, // Only checks if the entity has the component, retrieval returns nullptr
    Not, // Only selects entities without this component, retrieval returns nullptr
};

/// NOLINTEND
MECS_ENDEXTERNCPP()