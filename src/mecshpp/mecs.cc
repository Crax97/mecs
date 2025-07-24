#include "mecshpp/mecs.hpp"
#include "mecs/base.h"
#include "mecs/registry.h"
#include "mecs/world.h"

using namespace mecs;

Registry::Registry(const MecsRegistryCreateInfo& registryInfo)
{
    mReg = mecsRegistryCreate(&registryInfo);
}

Registry::~Registry()
{
    if (mReg == nullptr) { return; }
    mecsRegistryFree(mReg);
    mReg = nullptr;
}

ComponentID Registry::addRegistration(const ComponentInfo& info)
{
    return { mecsRegistryAddRegistration(mReg, &info) };
}

PrefabBuilder Registry::createPrefab()
{
    return { mReg, mecsRegistryCreatePrefab(mReg) };
}
void Registry::addPrefabComponent(PrefabID prefab, ComponentID component, const void* defaultValue)
{
    mecsRegistryPrefabAddComponentWithDefaults(mReg, prefab.id(), component.id(), defaultValue);
}

void* Registry::getPrefabComponent(PrefabID prefab, ComponentID component)
{
    return mecsRegistryPrefabGetComponent(mReg, prefab.id(), component.id());
}

void Registry::removePrefabComponent(PrefabID prefab, ComponentID component)
{
    mecsRegistryPrefabRemoveComponent(mReg, prefab.id(), component.id());
}

void Registry::destroyPrefab(PrefabID prefab)
{
    mecsRegistryDestroyPrefab(mReg, prefab.id());
}

World::World(Registry& reg, const MecsWorldCreateInfo& worldCreateInfo)
{
    mWorld = mecsWorldCreate(reg.getHandle(), &worldCreateInfo);
}

World::~World()
{
    if (mWorld == nullptr) { return; }
    mecsWorldFree(mWorld);
    mWorld = nullptr;
}

EntityBuilder World::spawnEntity(const MecsEntityInfo& entityInfo)
{
    return spawnEntityPrefab(PrefabID::invalid(), entityInfo);
}
EntityBuilder World::spawnEntityPrefab(PrefabID prefab, const MecsEntityInfo& entityInfo)
{
    return { mWorld, mecsWorldSpawnEntityPrefab(mWorld, prefab.id(), &entityInfo) };
}

bool World::entityHasComponent(EntityID entity, ComponentID component) const
{
    return mecsWorldEntityHasComponent(mWorld, entity.id(), component.id());
}

void* World::entityGetComponent(EntityID entity, ComponentID component) const
{
    return mecsWorldEntityGetComponent(mWorld, entity.id(), component.id());
}

void World::entityAddComponent(EntityID entity, ComponentID component)
{
    mecsWorldAddComponent(mWorld, entity.id(), component.id());
}

void World::entityRemoveComponent(EntityID entity, ComponentID component)
{
    mecsWorldRemoveComponent(mWorld, entity.id(), component.id());
}

void World::destroyEntity(EntityID entity)
{
    mecsWorldDestroyEntity(mWorld, entity.id());
}

void World::flushEvents()
{
    mecsWorldFlushEvents(mWorld);
}