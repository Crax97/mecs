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
MecsSize MECS_API mecsRegistryPrefabGetNumComponents(MecsRegistry* reg, MecsPrefabID prefabID);
MecsComponentID MECS_API mecsRegistryPrefabGetComponentIDByIndex(MecsRegistry* reg, MecsPrefabID prefabID, MecsSize componentIndex);
void* MECS_API mecsRegistryPrefabGetComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID);
void MECS_API mecsRegistryPrefabRemoveComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID);
void MECS_API mecsRegistryDestroyPrefab(MecsRegistry* reg, MecsPrefabID prefabID);
MecsSize MECS_API mecsRegistryGetNumPrefabs(MecsRegistry* reg);

MecsSize MECS_API mecsRegistryGetNumComponents(MecsRegistry* reg);

// Asserts if the index is not valid, otherwise it always returns a valid MecsComponentID
MecsComponentID MECS_API mecsGetComponentIDByIndex(MecsRegistry* reg, MecsSize index);

// return MECS_INVALID if no component was registered with this typeID, otherwise it always returns a valid MecsComponentID
MecsComponentID MECS_API mecsGetComponentIDByTypeID(MecsRegistry* reg, MecsTypeID typeID);

// Asserts if the index is not valid, otherwise it always returns a valid ComponentInfo
const MECS_API ComponentInfo* mecsGetComponentInfoByIndex(MecsRegistry* reg, MecsSize index);

// Asserts if the componentID is not valid, otherwise it always returns a valid ComponentInfo
const MECS_API ComponentInfo* mecsGetComponentInfoByComponentID(MecsRegistry* reg, MecsComponentID componentID);

// May return nullptr if there's no component with this typeID
const MECS_API ComponentInfo* mecsGetComponentInfoByTypeID(MecsRegistry* reg, MecsTypeID typeID);

MECS_ENDEXTERNCPP()