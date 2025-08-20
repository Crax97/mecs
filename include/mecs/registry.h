#pragma once
#include "base.h"
#include "defines.h"

MECS_EXTERNCPP()

MecsRegistry* MECS_API mecsRegistryCreate(const MecsRegistryCreateInfo* createInfo);
void MECS_API mecsRegistryFree(MecsRegistry* registry);
MecsComponentID MECS_API mecsRegistryAddRegistration(MecsRegistry* reg, const ComponentInfo* vtable);
MecsPrefabID MECS_API mecsRegistryCreatePrefab(MecsRegistry* reg);
void MECS_API mecsRegistryPrefabAddComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID);
void MECS_API mecsRegistryPrefabAddComponentWithDefaults(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID, const void* defaultValue);
void* MECS_API mecsRegistryPrefabGetComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID);
void MECS_API mecsRegistryPrefabRemoveComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID);
void MECS_API mecsRegistryDestroyPrefab(MecsRegistry* reg, MecsPrefabID prefabID);

MecsSize MECS_API mecsRegistryGetNumComponents(MecsRegistry* reg);
MecsComponentID MECS_API mecsGetComponentIDByIndex(MecsRegistry* reg, MecsSize index);
const MECS_API ComponentInfo* mecsGetComponentInfoByIndex(MecsRegistry* reg, MecsSize index);
const MECS_API ComponentInfo* mecsGetComponentInfoByID(MecsRegistry* reg, MecsComponentID componentID);

MECS_ENDEXTERNCPP()