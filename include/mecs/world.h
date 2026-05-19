#pragma once

#include "base.h"
#include "defines.h"

MECS_EXTERNCPP()

MECS_API MecsWorld* mecsWorldCreate(MecsRegistry* registry, const MecsWorldCreateInfo* mecsWorldCreateInfo);
MECS_API void mecsWorldFree(MecsWorld* world);
MECS_API MecsAllocator mecsWorldGetAllocator(MecsWorld* world);
MECS_API MecsEntityID mecsWorldSpawnEntity(MecsWorld* world, const MecsEntityInfo* entityInfo);
MECS_API MecsEntityID mecsWorldDuplicateEntity(MecsWorld* world, MecsWorld* destinationWorld, MecsEntityID entity);
MECS_API MecsEntityID mecsWorldSpawnEntityPrefab(MecsWorld* world, MecsPrefabID prefabID, const MecsEntityInfo* entityInfo);
MECS_API bool mecsWorldEntityHasComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component);
MECS_API void* mecsWorldEntityGetComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component);
MECS_API MecsSize mecsWorldEntityGetNumComponents(MecsWorld* world, MecsEntityID entity);
MECS_API MecsComponentID mecsWorldEntityGetComponentByIndex(MecsWorld* world, MecsEntityID entity, MecsSize index);
MECS_API void* mecsWorldAddComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component);
MECS_API void mecsWorldRemoveComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID componentInstance);
MECS_API void mecsWorldDestroyEntity(MecsWorld* world, MecsEntityID entityID);
MECS_API void mecsWorldEntityChanged(MecsWorld* world, MecsEntityID entityID);
MECS_API void mecsWorldFlushEvents(MecsWorld* world, void* updateData);
MECS_API MecsIterator* mecsWorldAcquireIterator(MecsWorld* world);
MECS_API void mecsWorldReleaseIterator(MecsWorld* world, MecsIterator* iterator);

// Utilities

/// These utility functions should be used very sparingly: always use MecsEntityID to interact with entities in the world.
/// Use these only when you need to pass an entityID to a system which isn't guaranteed to preserve the generation bits of
/// the EntityID
/// The main issue of these functions is that you could convert an entity's ID to an index, but you're not guaranteed
/// that converting the index back you get and ID that refers to the same entity

/// @brief Gets the underlying index of an entity.
/// @note Use sparingly: see the note above
MECS_API MecsU32 mecsEntityIDToIndex(MecsEntityID);
/// @brief Retrieves the MecsEntityID of an entity from it's index.
/// @returns MECS_INVALID if there's no such entity, otherwise a valid MecsEntityID
/// @note Use sparingly: see the note above
MECS_API MecsEntityID mecsWorldEntityIndexToID(MecsWorld* world, MecsU32 entityIndex);

// Systems
MECS_API MecsSystemID mecsWorldDefineSystem(MecsWorld* world, const MecsDefineSystemInfo* systemInfo);

MECS_API MecsRegistry* mecsWorldGetRegistry(MecsWorld* world);

MECS_ENDEXTERNCPP()
