#include "mecshpp/mecs.hpp"
#include "mecs/base.h"
#include "mecs/registry.h"
#include "mecs/world.h"

using namespace mecs;

Registry::Registry(const MecsRegistryCreateInfo& registryInfo)
{
    mHandle = mecsRegistryCreate(&registryInfo);
}

Registry::~Registry()
{
    if (mHandle == nullptr) { return; }
    mecsRegistryFree(mHandle);
    mHandle = nullptr;
}

ComponentID Registry::addRegistration(const ComponentInfo& info)
{
    return { mecsRegistryAddRegistration(mHandle, &info) };
}

PrefabBuilder Registry::createPrefab()
{
    return { mHandle, mecsRegistryCreatePrefab(mHandle) };
}
void Registry::addPrefabComponent(PrefabID prefab, ComponentID component, const void* defaultValue)
{
    mecsRegistryPrefabAddComponentWithDefaults(mHandle, prefab.id(), component.id(), defaultValue);
}

void* Registry::getPrefabComponent(PrefabID prefab, ComponentID component)
{
    return mecsRegistryPrefabGetComponent(mHandle, prefab.id(), component.id());
}

void Registry::removePrefabComponent(PrefabID prefab, ComponentID component)
{
    mecsRegistryPrefabRemoveComponent(mHandle, prefab.id(), component.id());
}

void Registry::destroyPrefab(PrefabID prefab)
{
    mecsRegistryDestroyPrefab(mHandle, prefab.id());
}
MecsSize Registry::getPrefabNumComponents(PrefabID prefabID) const
{
    return mecsRegistryPrefabGetNumComponents(mHandle, prefabID.id());
}
ComponentID Registry::getPrefabComponentIDByIndex(PrefabID prefabID, MecsSize componentIndex) const
{
    return { mecsRegistryPrefabGetComponentIDByIndex(mHandle, prefabID.id(), componentIndex) };
}
MecsSize Registry::getNumPrefabs() const
{
    return mecsRegistryGetNumPrefabs(mHandle);
}
MecsSize Registry::getNumComponents() const
{
    return mecsRegistryGetNumComponents(mHandle);
}
mecs::ComponentID Registry::getComponentIDByName(const std::string& name) const
{
    return { mecsGetComponentIDByName(mHandle, name.c_str()) };
}

mecs::ComponentID Registry::getComponentIDByIndex(MecsSize index) const
{
    return { mecsGetComponentIDByIndex(mHandle, index) };
}
const ComponentInfo& Registry::getComponentInfoByIndex(MecsSize index) const
{
    return *mecsGetComponentInfoByIndex(mHandle, index);
}
const ComponentInfo& Registry::getComponentInfoByComponentID(mecs::ComponentID componentID) const
{
    return *mecsGetComponentInfoByComponentID(mHandle, componentID.id());
}

World::World(Registry& reg, const MecsWorldCreateInfo& worldCreateInfo)
{
    mHandle = mecsWorldCreate(reg.getHandle(), &worldCreateInfo);
}

World::~World()
{
    if (mHandle == nullptr) { return; }
    mecsWorldFree(mHandle);
    mHandle = nullptr;
}

EntityBuilder World::spawnEntity(const MecsEntityInfo& entityInfo)
{
    return spawnEntityPrefab(PrefabID::invalid(), entityInfo);
}
EntityBuilder World::spawnEntityPrefab(PrefabID prefab, const MecsEntityInfo& entityInfo)
{
    return { mHandle, mecsWorldSpawnEntityPrefab(mHandle, prefab.id(), &entityInfo) };
}

bool World::entityHasComponent(EntityID entity, ComponentID component) const
{
    return mecsWorldEntityHasComponent(mHandle, entity.id(), component.id());
}

void* World::entityGetComponent(EntityID entity, ComponentID component) const
{
    return mecsWorldEntityGetComponent(mHandle, entity.id(), component.id());
}

void World::entityAddComponent(EntityID entity, ComponentID component)
{
    mecsWorldAddComponent(mHandle, entity.id(), component.id());
}

void World::entityRemoveComponent(EntityID entity, ComponentID component)
{
    mecsWorldRemoveComponent(mHandle, entity.id(), component.id());
}
MecsSize World::entityGetNumComponents(EntityID entity) const
{
    return mecsWorldEntityGetNumComponents(mHandle, entity.id());
}
ComponentID World::entityGetComponentByIndex(EntityID entity, MecsSize index) const
{
    return { mecsWorldEntityGetComponentByIndex(mHandle, entity.id(), index) };
}
void World::destroyEntity(EntityID entity)
{
    mecsWorldDestroyEntity(mHandle, entity.id());
}

void World::flushEvents()
{
    mecsWorldFlushEvents(mHandle);
}