#include "mecs/registry.h"
#include "mecs/base.h"

#include "private.h"
#include <cstring>

MecsRegistry* mecsRegistryCreate(const MecsRegistryCreateInfo* createInfo)
{

    MecsAllocator allocator;
    if (createInfo != nullptr && createInfo->memAllocator.memAlloc != nullptr) {
        allocator = createInfo->memAllocator;
    } else {
        allocator = kDefaultAllocator;
    }

    MecsRegistry* reg = mecsAlloc<MecsRegistry>(allocator, allocator);
    reg->memAllocator = allocator;

    return reg;
}

void updateComponentInfo(ComponentInfo& dest, const ComponentInfo& source)
{
    const char* nameCopy = dest.name;
    dest = source;
    dest.name = nameCopy;
}

MecsComponentID
mecsRegistryAddRegistration(MecsRegistry* reg,
    const ComponentInfo* vtable)
{
    MECS_ASSERT(reg != nullptr && vtable != nullptr);
    MECS_ASSERT(vtable->size > 0 && vtable->align > 0 && vtable->name != nullptr);

    MecsSize elementCount = reg->components.count();
    for (MecsSize i = 0; i < elementCount; i++) {
        ComponentInfo& info = reg->components[i];

        if (info.typeID == vtable->typeID) {
            MECS_ASSERT(mecsStrEqual(info.name, vtable->name) && "A component was registered with an already existing typeID, but with a different name: two components sharing the same typeID must have the same name");

            updateComponentInfo(info, *vtable);
            return static_cast<MecsComponentID>(i);
        }
    }

    ComponentInfo info = *vtable;
    info.name = mecsStrDup(reg->memAllocator, vtable->name);

    MECS_ASSERT(info.name != vtable->name);

    MecsSize size = reg->components.count();
    reg->components.push(reg->memAllocator, info);
    return size;
}

void mecsRegistryFree(MecsRegistry* registry)
{
    if (registry == nullptr) {
        return;
    }

    registry->prefabs.forEach([&]([[maybe_unused]]
                                  MecsPrefabID pid,
                                  MecsPrefab& prefab) {
        prefab.components.forEach([&](MecsPrefabComponent& component) {
            component.blob.destroy(registry);
        });
        prefab.components.destroy(registry->memAllocator);
        prefab.archetypeBitset.destroy(registry->memAllocator);
    });
    registry->prefabs.destroy(registry->memAllocator);

    const MecsSize numComponents = registry->components.count();
    for (MecsSize i = 0; i < numComponents; i++) {
        ComponentInfo& info = registry->components[i];
        mecsFree(registry->memAllocator, info.name);
    }
    registry->components.destroy(registry->memAllocator);

    mecsFree(registry->memAllocator, registry);
}

MecsPrefabID mecsRegistryCreatePrefab(MecsRegistry* reg)
{
    MECS_ASSERT(reg != nullptr);
    return reg->prefabs.push(reg->memAllocator, {});
}

MecsSize mecsRegistryGetNumPrefabs(MecsRegistry* reg)
{
    MECS_ASSERT(reg != nullptr);
    return reg->prefabs.count();
}

MecsSize mecsRegistryPrefabGetNumComponents(MecsRegistry* reg, MecsPrefabID prefabID)
{
    MECS_ASSERT(reg != nullptr);
    MecsPrefab* prefab = reg->prefabs.at(prefabID);
    return prefab->components.count();
}
MecsComponentID mecsRegistryPrefabGetComponentIDByIndex(MecsRegistry* reg, MecsPrefabID prefabID, MecsSize componentIndex)
{
    MECS_ASSERT(reg != nullptr);
    MecsPrefab* prefab = reg->prefabs.at(prefabID);
    return prefab->components[componentIndex].component;
}

void mecsRegistryPrefabAddComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID)
{
    mecsRegistryPrefabAddComponentWithDefaults(reg, prefabID, componentID, nullptr);
}
void mecsRegistryPrefabAddComponentWithDefaults(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID, const void* defaultValue)
{
    MECS_ASSERT(reg != nullptr);

    MecsPrefab* pPrefab = reg->prefabs.at(prefabID);
    MECS_ASSERT(pPrefab != nullptr);
    MecsPrefab& prefab = *pPrefab;
    const ComponentInfo& info = reg->components[componentID];
    void* blobPtr = nullptr;
    MecsSize componentCount = prefab.components.count();
    for (MecsSize i = 0; i < componentCount; i++) {
        MecsPrefabComponent& component = prefab.components[i];
        if (component.component == componentID) {
            blobPtr = component.blob.get();
            break;
        }
    }

    if (blobPtr == nullptr) {
        ComponentBlob blob(reg, componentID);
        blobPtr = blob.get();
        prefab.components.push(reg->memAllocator, { .component = componentID, .blob = std::move(blob) });
    }
    MECS_ASSERT(blobPtr != nullptr);
    if (defaultValue != nullptr) {
        if (info.copy != nullptr) {
            info.copy(defaultValue, blobPtr, info.size);
        } else {
            memcpy(blobPtr, defaultValue, info.size);
        }
    } else {
        if (info.init != nullptr) {
            info.init(blobPtr);
        } else {
            memset(blobPtr, 0, info.size);
        }
    }
    prefab.archetypeBitset.set(reg->memAllocator, componentID, true);
}
void* mecsRegistryPrefabGetComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID)
{
    MECS_ASSERT(reg != nullptr);

    MecsPrefab* pPrefab = reg->prefabs.at(prefabID);
    MECS_ASSERT(pPrefab != nullptr);
    MecsPrefab& prefab = *pPrefab;
    MecsSize componentCount = prefab.components.count();
    const ComponentInfo& info = reg->components[componentID];
    for (MecsSize i = 0; i < componentCount; i++) {
        MecsPrefabComponent& component = prefab.components[i];
        if (component.component == componentID) {
            void* blobPtr = component.blob.get();
            return blobPtr;
        }
    }
    MECS_ASSERT(false && "Component not found");
    return nullptr;
}
void mecsRegistryPrefabRemoveComponent(MecsRegistry* reg, MecsPrefabID prefabID, MecsComponentID componentID)
{
    MECS_ASSERT(reg != nullptr);
    MecsPrefab* pPrefab = reg->prefabs.at(prefabID);
    MECS_ASSERT(pPrefab != nullptr);
    MecsPrefab& prefab = *pPrefab;
    MECS_ASSERT(prefab.archetypeBitset.test(componentID) && "Component not found");

    MecsSize componentCount = prefab.components.count();
    const ComponentInfo& info = reg->components[componentID];
    prefab.archetypeBitset.set(reg->memAllocator, componentID, false);
    for (MecsSize i = 0; i < componentCount; i++) {
        MecsPrefabComponent& component = prefab.components[i];
        if (component.component == componentID) {
            prefab.components[i].blob.destroy(reg);
            prefab.components.removeAt(i);
            return;
        }
    }
}
void mecsRegistryDestroyPrefab(MecsRegistry* reg, MecsPrefabID prefabID)
{
    MECS_ASSERT(reg != nullptr);
    MecsPrefab* pPrefab = reg->prefabs.at(prefabID);
    MECS_ASSERT(pPrefab != nullptr);
    MecsPrefab& prefab = *pPrefab;

    prefab.components.forEach([&](MecsPrefabComponent& component) {
        component.blob.destroy(reg);
    });
    prefab.components.destroy(reg->memAllocator);
    prefab.archetypeBitset.destroy(reg->memAllocator);

    reg->prefabs.remove(reg->memAllocator, prefabID);
}

MecsSize mecsRegistryGetNumComponents(MecsRegistry* reg)
{
    MECS_ASSERT(reg != nullptr && "reg must not be null");
    return reg->components.count();
}

MecsComponentID mecsGetComponentIDByName(MecsRegistry* reg, const char* name)
{
    MECS_ASSERT(reg != nullptr && "reg must not be null");

    MecsSize numComponents = reg->components.count();
    for (MecsSize i = 0; i < numComponents; i++) {
        const ComponentInfo& component = reg->components[i];
        if (mecsStrEqual(component.name, name)) {
            return static_cast<MecsComponentID>(i);
        }
    }
    return MECS_INVALID;
}

MecsComponentID mecsGetComponentIDByIndex(MecsRegistry* reg, MecsSize index)
{
    MECS_ASSERT(reg != nullptr && "reg must not be null");
    MECS_ASSERT(reg->components.isValid(index) && "Invalid index");
    return static_cast<MecsComponentID>(index);
}

const ComponentInfo* mecsGetComponentInfoByIndex(MecsRegistry* reg, MecsSize index)
{
    MECS_ASSERT(reg != nullptr && "reg must not be null");
    return reg->components.atPtr(index);
}
const ComponentInfo* mecsGetComponentInfoByComponentID(MecsRegistry* reg, MecsComponentID componentID)
{
    MECS_ASSERT(reg != nullptr && "reg must not be null");
    return reg->components.atPtr(componentID);
}
