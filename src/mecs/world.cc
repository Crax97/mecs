
#include "mecs/world.h"

#include "collections.h"
#include "mecs/base.h"
#include "private.h"

#include <cassert>

constexpr MecsSize kComponentInstanceIDNumBits = 24;
constexpr MecsSize kComponentInstanceMask = ((1 << kComponentInstanceIDNumBits) - 1);

void freeEntityRow(MecsWorld* const world, const MecsEntity& ent)
{
    Archetype& oldArchetype = world->archetypes[ent.archetype];
    MecsSize replacementRow = oldArchetype.storage.freeRow(world->memAllocator, ent.archetypeRow);
    MecsEntityID entityRowReplaced = oldArchetype.rowToEntity[replacementRow];
    {
        MecsEntity* rowReplacementEntity = world->entities.at(entityRowReplaced);
        rowReplacementEntity->archetypeRow = ent.archetypeRow; // This entity now points to the row of the old entity
        oldArchetype.rowToEntity[ent.archetypeRow] = entityRowReplaced;
        oldArchetype.rowToEntity.pop();
    }
}

void mecsOnNewEntitySpawned(MecsWorld* const& world, MecsEntityID entityID)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");

    ent->status = EntityStatus::eSpawned;
}

void mecsOnComponentAddedToEntity(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID component)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");
}

void mecsOnComponentRemovedFromEntity(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID component)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");
}

void mecsOnEntityDestroyed(MecsWorld* world, MecsEntityID entityID)
{
    MecsEntity* ent = world->entities.at(entityID);
    freeEntityRow(world, *ent);
    world->entities.remove(world->memAllocator, entityID);
}

void mecsOnNewArchetype(MecsWorld* world, ArchetypeID archetypeID)
{
    Archetype& entArchetype = world->archetypes[archetypeID];
    world->acquiredIterators.forEach([&](MecsIterator* iterator) {
        if (iterator->componentSet.contains(entArchetype.storage.bitset())) {
            iterator->archetypes.pushUnique(world->memAllocator, archetypeID);
        }
    });
}

ArchetypeID findArchetype(MecsWorld* world, const BitSet& archetypeBitset)
{
    if (archetypeBitset.allZeroes()) {
        return MECS_INVALID;
    }

    MecsSize numArchetypes = world->archetypes.count();
    for (MecsSize i = 0; i < numArchetypes; i++) {
        Archetype& testArchetype = world->archetypes[i];
        if (testArchetype.storage.bitset() == archetypeBitset) {
            return i;
        }
    }

    ArchetypeID archID = world->archetypes.count();
    RowStorage storage = RowStorage { std::move(archetypeBitset.clone(world->memAllocator)), world };
    world->archetypes.push(world->memAllocator, { .storage = std::move(storage) });

    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eNewArchetype,
                                                   .entityID = archID,
                                               });
    return archID;
}

ArchetypeID findNewArchetype(MecsWorld* world, ArchetypeID source, MecsComponentID component, bool include)
{
    BitSet archetypeBitset;

    if (source != MECS_INVALID) {
        Archetype& sourceArchetype = world->archetypes[source];
        archetypeBitset = sourceArchetype.storage.bitset().clone(world->memAllocator);
    }
    archetypeBitset.set(world->memAllocator, component, include);

    ArchetypeID archetypeID = findArchetype(world, archetypeBitset);
    archetypeBitset.destroy(world->memAllocator);

    return archetypeID;
}

void moveEntityToNewArchetype(MecsWorld* const world, MecsEntityID entity, ArchetypeID newArchetypeID)
{

    MecsEntity* ent = world->entities.at(entity);
    MECS_ASSERT(newArchetypeID != ent->archetype);

    if (newArchetypeID == MECS_INVALID) {
        freeEntityRow(world, *ent);
        return;
    }

    Archetype& newArchetype = world->archetypes[newArchetypeID];

    MecsSize newRow = newArchetype.storage.allocateRow(world->memAllocator);

    if (ent->archetype != MECS_INVALID) {
        Archetype& oldArchetype = world->archetypes[ent->archetype];
        oldArchetype.storage.copyRow(ent->archetypeRow, newArchetype.storage, newRow);

        freeEntityRow(world, *ent);
    }
    newArchetype.rowToEntity.ensureSize(world->memAllocator, newRow + 1);
    newArchetype.rowToEntity[newRow] = entity;

    ent->archetype = newArchetypeID;
    ent->archetypeRow = newRow;
}

MecsWorld*
mecsWorldCreate(MecsRegistry* registry, const MecsWorldCreateInfo* const mecsWorldCreateInfo)
{
    assert(registry != nullptr);

    MecsAllocator allocator;
    if (mecsWorldCreateInfo != nullptr && mecsWorldCreateInfo->memAllocator.memAlloc != nullptr) {
        allocator = mecsWorldCreateInfo->memAllocator;
    } else {
        allocator = registry->memAllocator;
    }

    MecsWorld* world = mecsAlloc<MecsWorld>(allocator);

    world->registry = registry;
    world->memAllocator = allocator;

    return world;
}

void mecsIteratorReleaseResources(const MecsAllocator& alloc, MecsIterator* iter)
{
    iter->components.destroy(alloc);
    iter->componentSet.destroy(alloc);
    iter->blacklistComponentSet.destroy(alloc);
    iter->archetypes.destroy(alloc);
    mecsFree(alloc, iter);
}

void mecsWorldFree(MecsWorld* world)
{
    if (world == nullptr) {
        return;
    }

    mecsWorldFlushEvents(world);

    world->archetypes.forEach([world](Archetype& bucket) {
        bucket.storage.destroy(world->memAllocator);
        bucket.rowToEntity.destroy(world->memAllocator);
    });
    world->archetypes.destroy(world->memAllocator);
    world->entities.destroy(world->memAllocator);

    world->acquiredIterators.forEach([world](auto& iter) {
        mecsIteratorReleaseResources(world->memAllocator, iter);
    });

    world->reusableIterators.forEach([world](auto& iter) {
        mecsIteratorReleaseResources(world->memAllocator, iter);
    });

    world->acquiredIterators.destroy(world->memAllocator);
    world->reusableIterators.destroy(world->memAllocator);
    world->newEvents.destroy(world->memAllocator);
    mecsFree(world->memAllocator, world);
}

MecsEntityID mecsWorldSpawnEntity(MecsWorld* const world, const MecsEntityInfo* entityInfo)
{
    return mecsWorldSpawnEntityPrefab(world, MECS_INVALID, nullptr);
}

void setupEntityThroughPrefab(MecsWorld*& world, MecsPrefabID& prefabID, MecsEntityID entityID, MecsEntity& ent)
{
    const MecsPrefab* pPrefab = world->registry->prefabs.at(prefabID);
    MECS_ASSERT(pPrefab != nullptr && "Invalid Prefab ID");
    const MecsPrefab& prefab = *pPrefab;
    if (prefab.archetypeBitset.allZeroes()) {
        return;
    }
    // Setup entity by copying from the prefab
    ArchetypeID entityArchetype = findArchetype(world, prefab.archetypeBitset);
    ent.archetype = entityArchetype;
    if (entityArchetype != MECS_INVALID) {
        Archetype& archetype = world->archetypes[entityArchetype];
        MecsSize row = archetype.storage.allocateRow(world->memAllocator);
        prefab.components.forEach([&](const MecsPrefabComponent& component) {
            void* componentPtr = archetype.storage.getRowComponent(component.component, row);
            component.blob.copyOnto(world->registry, componentPtr);
        });
        ent.archetypeRow = row;
        archetype.rowToEntity.ensureSize(world->memAllocator, row + 1);
        archetype.rowToEntity[row] = entityID;
    }
}
MecsEntityID mecsWorldSpawnEntityPrefab(MecsWorld* world, MecsPrefabID prefabID, const MecsEntityInfo* entityInfo)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity ent = {};
    if (entityInfo != nullptr) {
        if (ent.name != nullptr) {
            ent.name = mecsStrDup(world->memAllocator, ent.name);
        }
    }
    ent.status = EntityStatus::eNewlySpawned;
    ent.archetype = MECS_INVALID;
    ent.archetypeRow = MECS_INVALID;
    ent.prefabID = prefabID;
    MecsEntityID entityID = world->entities.push(world->memAllocator, {});

    if (prefabID != MECS_INVALID) {
        setupEntityThroughPrefab(world, prefabID, entityID, ent);
    }

    *world->entities.at(entityID) = ent;

    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eNewEntity,
                                                   .entityID = entityID,
                                               });

    return entityID;
}

void* mecsWorldAddComponent(MecsWorld* const world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(entity != MECS_INVALID && "Invalid entity ID");
    MecsEntity* ent = world->entities.at(entity);
    MECS_ASSERT(ent->status != EntityStatus::eDestroying);
    ComponentInfo info = world->registry->components[component];

    void* outPtr = nullptr;
    if (ent->archetype == MECS_INVALID) {
        ArchetypeID newArchetypeID = findNewArchetype(world, ent->archetype, component, true);
        moveEntityToNewArchetype(world, entity, newArchetypeID);
        Archetype& newArchetype = world->archetypes[newArchetypeID];
        outPtr = newArchetype.storage.getRowComponent(component, ent->archetypeRow);

        world->newEvents.push(world->memAllocator, WorldEvent {
                                                       .kind = WorldEventKind::eNewComponent,
                                                       .entityID = entity,
                                                       .componentID = component,
                                                   });
    } else {
        Archetype& oldArchetype = world->archetypes[ent->archetype];

        if (oldArchetype.storage.hasComponent(component)) {
            outPtr = oldArchetype.storage.getRowComponent(component, ent->archetypeRow);
            if (info.destroy != nullptr) { info.destroy(outPtr); }
            world->newEvents.push(world->memAllocator, WorldEvent {
                                                           .kind = WorldEventKind::eUpdateComponent,
                                                           .entityID = entity,
                                                           .componentID = component,
                                                       });
        } else {
            ArchetypeID newArchetypeID = findNewArchetype(world, ent->archetype, component, true);
            moveEntityToNewArchetype(world, entity, newArchetypeID);
            Archetype& newArchetype = world->archetypes[newArchetypeID];
            outPtr = newArchetype.storage.getRowComponent(component, ent->archetypeRow);
            world->newEvents.push(world->memAllocator, WorldEvent {
                                                           .kind = WorldEventKind::eNewComponent,
                                                           .entityID = entity,
                                                           .componentID = component,
                                                       });
        }
    }

    if (info.init != nullptr) {
        info.init(outPtr);
    }

    return outPtr;
}

bool mecsWorldEntityHasComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(entity != MECS_INVALID && "Invalid entity ID");

    MecsEntity* ent = world->entities.at(entity);
    ComponentInfo info = world->registry->components[component];
    const Archetype& archetype = world->archetypes[ent->archetype];
    return archetype.storage.hasComponent(component);
}
void* mecsWorldEntityGetComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(entity != MECS_INVALID && "Invalid entity ID");

    MecsEntity* ent = world->entities.at(entity);
    ComponentInfo info = world->registry->components[component];
    const Archetype& archetype = world->archetypes[ent->archetype];
    MECS_ASSERT(archetype.storage.hasComponent(component));
    return archetype.storage.getRowComponent(component, ent->archetypeRow);
}
void mecsWorldRemoveComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);

    void* pComponent = nullptr;

    MecsEntity* ent = world->entities.at(entity);
    ArchetypeID newArchetypeID = findNewArchetype(world, ent->archetype, component, false);
    if (ent->archetype == newArchetypeID) {
        return; // The entity does not have the component;
    }
    moveEntityToNewArchetype(world, entity, newArchetypeID);
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eDestroyComponent,
                                                   .entityID = entity,
                                                   .componentID = component,
                                               });
}
void mecsWorldDestroyEntity(MecsWorld* const world, MecsEntityID entityID)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr);
    if (ent->status == EntityStatus::eDestroying) { return; }
    ent->status = EntityStatus::eDestroying;
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eDestroyEntity,
                                                   .entityID = entityID,
                                               });
}

void mecsWorldFlushEvents(MecsWorld* world)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    world->newEvents.forEach([&](const WorldEvent& event) {
        switch (event.kind) {
        case WorldEventKind::eNewEntity: {
            mecsOnNewEntitySpawned(world, event.entityID);
            break;
        }
        case WorldEventKind::eDestroyEntity: {
            mecsOnEntityDestroyed(world, event.entityID);
            break;
        }
        case WorldEventKind::eNewComponent: {
            mecsOnComponentAddedToEntity(world, event.entityID, event.componentID);
            break;
        }
        case WorldEventKind::eUpdateComponent: {
            // For now do nothing
            break;
        }
        case WorldEventKind::eDestroyComponent: {
            mecsOnComponentRemovedFromEntity(world, event.entityID, event.componentID);
            break;
        };
        case WorldEventKind::eNewArchetype: {
            mecsOnNewArchetype(world, event.entityID);
            break;
        }
        }
    });
    world->newEvents.clear();
}

MecsIterator* mecsWorldAcquireIterator(MecsWorld* world)
{
    MecsIterator* itr = nullptr;
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    if (!world->reusableIterators.empty()) {
        itr = world->reusableIterators.pop();
    } else {
        itr = mecsAlloc<MecsIterator>(world->memAllocator);
        itr->world = world;
    }

    world->acquiredIterators.push(world->memAllocator, itr);
    MECS_ASSERT(itr->status == IteratorStatus::eReleased);
    itr->status = IteratorStatus::eInitializing;
    return itr;
}

void mecsWorldReleaseIterator(MecsWorld* world, MecsIterator* iterator)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MECS_ASSERT(iterator->status != IteratorStatus::eReleased && "Releasing an iterator twice");
    iterator->status = IteratorStatus::eReleased;

    iterator->components.clear();
    iterator->componentSet.clear();
    iterator->blacklistComponentSet.clear();
    iterator->archetypes.clear();
    world->reusableIterators.push(world->memAllocator, iterator);
    bool removed = world->acquiredIterators.remove(iterator);
    MECS_ASSERT(removed);
}

MecsRegistry* mecsWorldGetRegistry(MecsWorld* world)
{
    MECS_ASSERT(world != nullptr && "World must not be null");
    return world->registry;
}