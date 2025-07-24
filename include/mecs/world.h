#pragma once

#include "base.h"
#include "defines.h"

MECS_EXTERNCPP()

MECS_API MecsWorld* mecsWorldCreate(const MecsWorldCreateInfo* mecsWorldCreateInfo);
MECS_API void mecsWorldFree(MecsWorld* world);
MECS_API MecsEntityID mecsWorldSpawnEntity(MecsWorld* world, const MecsEntityInfo* entityInfo);
MECS_API bool mecsWorldEntityHasComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component);
MECS_API void* mecsWorldAddComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component);
MECS_API void mecsWorldRemoveComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID componentInstance);
MECS_API void mecsWorldDestroyEntity(MecsWorld* world, MecsEntityID entityID);
MECS_API MecsIterator* mecsWorldAcquireIterator(MecsWorld* world);
MECS_API void mecsWorldReleaseIterator(MecsWorld* world, MecsIterator* iterator);
MECS_API void mecsWorldFlushEvent(MecsWorld* world);

MECS_ENDEXTERNCPP()