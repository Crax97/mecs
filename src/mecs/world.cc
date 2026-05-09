
#include "mecs/world.h"
#include "mecs/iterator.h"

#include "collections.h"
#include "mecs/base.h"
#include "private.h"

#include <cassert>

constexpr MecsSize kComponentInstanceIDNumBits = 24;
constexpr MecsSize kComponentInstanceMask = ((1 << kComponentInstanceIDNumBits) - 1);

void mecsWorldRunSystems(MecsWorld* world, void* updateData);
void moveEntityToNewArchetype(MecsWorld* world, MecsEntityID entity, ArchetypeID newArchetypeID);

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

void mecsOnNewEntitySpawned(MecsWorld* const& world, MecsEntityID entityID, void* updateData)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");

    ent->status = EntityStatus::eSpawned;
}

void mecsAddEntityToNewMatchingSystems(MecsWorld* world,  void* updateData, MecsEntityID entityID, const BitSet& oldEntityComponentBitset, const BitSet& newEntityBitset)
{
    world->systems.forEach([&](MecsSystem& system) {
        if (system.onEntityAdded == nullptr) { return; }

        const Archetype& systemArchetype = world->archetypes[system.systemArchetype];
        const BitSet& systemBitset = systemArchetype.storage.bitset();

        // If a system archetype did not include the old entity archetype
        // but does include the new entity archetype
        // it means the system now matches the entity
        const bool entityWasNotIncludedBefore = !systemBitset.contains(oldEntityComponentBitset);
        const bool entityIsIncludedNow = systemBitset.contains(newEntityBitset);
        if (entityWasNotIncludedBefore && entityIsIncludedNow) {
            system.onEntityAdded(system.systemData, updateData, entityID);
        }
    });
}

void mecsRemoveEntityFromUnmatchingSystems(MecsWorld* world,  void* updateData, MecsEntityID entityID, const BitSet& oldEntityComponentBitset, const BitSet& newEntityBitset)
{
    world->systems.forEach([&](MecsSystem& system) {
        if (system.onEntityRemoved == nullptr) { return; }

        const Archetype& systemArchetype = world->archetypes[system.systemArchetype];
        const BitSet& systemBitset = systemArchetype.storage.bitset();

        // If a system archetype did include the old entity archetype
        // but does not include the new entity archetype anymore
        // it means the system does not match the entity anymore
        const bool entityWasIncludedBefore = systemBitset.contains(oldEntityComponentBitset);
        const bool entityIsNotIncludedNow = !systemBitset.contains(newEntityBitset);
        if (entityWasIncludedBefore && entityIsNotIncludedNow) {
            system.onEntityRemoved(system.systemData, updateData, entityID);
        }
    });
}

void mecsOnComponentAddedToEntity(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID componentID, ArchetypeID oldArchetypeID, ArchetypeID newArchetypeID, void* updateData)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to mecsOnComponentAddedToEntity");
    auto& componentInfo = world->registry->components.at(componentID);
    if (componentInfo.setup != nullptr) { componentInfo.setup(world, entityID, mecsWorldEntityGetComponent(world, entityID, componentID), updateData); }
    const Archetype& oldArchetype = world->archetypes[oldArchetypeID];
    const Archetype& newArchetype = world->archetypes[newArchetypeID];
    mecsAddEntityToNewMatchingSystems(world, updateData, entityID, oldArchetype.storage.bitset(), newArchetype.storage.bitset());
}

void mecsOnComponentRemovedFromEntity(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID componentID, void* updateData)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");
    const MecsRegistry* registry = world->registry;
    auto& componentInfo = registry->components.at(componentID);
    if (componentInfo.teardown != nullptr) { componentInfo.teardown(world, entityID, mecsWorldEntityGetComponent(world, entityID, componentID), updateData); }

    ArchetypeID oldArchetypeID = ent->archetype;
    ArchetypeID newArchetypeID = findNewArchetype(world, ent->archetype, componentID, false);
    if (oldArchetypeID == newArchetypeID) {
        return; // The entity does not have the component;
    }

    const Archetype& oldArchetype = world->archetypes[oldArchetypeID];
    const Archetype& newArchetype = world->archetypes[newArchetypeID];
    mecsRemoveEntityFromUnmatchingSystems(world, updateData, entityID, oldArchetype.storage.bitset(), newArchetype.storage.bitset());


    moveEntityToNewArchetype(world, entityID, newArchetypeID);

}

void mecsOnEntityDestroyed(MecsWorld* world, MecsEntityID entityID, void* updateData)
{
    MecsEntity* ent = world->entities.at(entityID);
    const ArchetypeID archetypeID = ent->archetype;
    const Archetype& entityArchetype = world->archetypes.at(archetypeID);
    const MecsRegistry* registry = world->registry;
    entityArchetype.componentIDs.forEach([&](MecsComponentID componentID) {
        auto& componentInfo = registry->components.at(componentID);
        if (componentInfo.teardown != nullptr) { componentInfo.teardown(world, entityID, mecsWorldEntityGetComponent(world, entityID, componentID), updateData); }
    });

    const Archetype& oldArchetype = world->archetypes[archetypeID];
    BitSet emptyBitset;
    mecsRemoveEntityFromUnmatchingSystems(world, updateData, entityID, oldArchetype.storage.bitset(), emptyBitset);
    emptyBitset.destroy(world->memAllocator);

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
    MecsSize numArchetypes = world->archetypes.count();
    for (MecsSize i = 0; i < numArchetypes; i++) {
        Archetype& testArchetype = world->archetypes[i];
        if (testArchetype.storage.bitset() == archetypeBitset) {
            return i;
        }
    }

    ArchetypeID archID = world->archetypes.count();
    MecsVec<MecsComponentID> components;
    archetypeBitset.forEach([&](MecsSize idx) {
        components.push(world->memAllocator, static_cast<MecsComponentID>(idx));
    });

    RowStorage storage = RowStorage { std::move(archetypeBitset.clone(world->memAllocator)), world };

    world->archetypes.push(world->memAllocator, { .storage = std::move(storage), .componentIDs = std::move(components) });

    mecsOnNewArchetype(world, archID);

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

    auto* world = mecsAlloc<MecsWorld>(allocator);

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

    world->systems.forEach([world](MecsSystem& system) {
        mecsWorldReleaseIterator(world, system.systemIterator);
    });
    world->systems.destroy(world->memAllocator);
    world->archetypes.forEach([world](Archetype& bucket) {
        bucket.storage.destroy(world->memAllocator);
        bucket.componentIDs.destroy(world->memAllocator);
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
MECS_API MecsAllocator mecsWorldGetAllocator(MecsWorld* world)
{
    MECS_ASSERT(world);
    return world->memAllocator;
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
    } else {
        BitSet bitset;
        const ArchetypeID defaultArchetype = findArchetype(world, bitset);
        bitset.destroy(world->memAllocator);

        Archetype& archetype = world->archetypes[defaultArchetype];
        const MecsSize row = archetype.storage.allocateRow(world->memAllocator);
        ent.archetypeRow = row;
        ent.archetype = defaultArchetype;
        archetype.rowToEntity.ensureSize(world->memAllocator, row + 1);
        archetype.rowToEntity[row] = entityID;
    }

    *world->entities.at(entityID) = ent;

    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eNewEntity,
                                                   .entityID = entityID,
                                               });

    return entityID;
}

MECS_API MecsEntityID mecsWorldDuplicateEntity(MecsWorld* world, MecsWorld* destinationWorld, MecsEntityID entity)
{
    MECS_ASSERT(mecsWorldGetRegistry(world) == mecsWorldGetRegistry(destinationWorld) && "source and destination worlds must have been spawned by the same registry");

    MecsRegistry* registry = mecsWorldGetRegistry(world);
    MecsEntityID newEntityID = destinationWorld->entities.push(destinationWorld->memAllocator, {});

    ArchetypeID destArchetypeID;
    MecsSize destEntityRow;
    // The reason this is done in two steps is because
    // when world == destinationWorld, we might end up modifying the entities and archetype arrays while
    // there's a valid reference to the sourceEntity.
    // By reacquiring the sourceEntity in the second step, we make sure to acquire a new valid reference to it
    {
        MecsEntity* source = world->entities.at(entity);
        const Archetype& sourceArch = world->archetypes.at(source->archetype);

        destArchetypeID = findArchetype(destinationWorld, sourceArch.storage.bitset());
        Archetype& destArch = destinationWorld->archetypes.at(destArchetypeID);
        destEntityRow = destArch.storage.allocateRow(destinationWorld->memAllocator);
        destArch.rowToEntity.ensureSize(world->memAllocator, destEntityRow + 1);
        destArch.rowToEntity[destEntityRow] = newEntityID;
    }

    {
        MecsEntity* source = world->entities.at(entity);
        MecsEntity destEntity = {};
        if (source->name != nullptr) {
            destEntity.name = mecsStrDup(world->memAllocator, source->name);
        }
        destEntity.status = EntityStatus::eNewlySpawned;
        destEntity.prefabID = source->prefabID;
        destEntity.archetype = destArchetypeID;
        destEntity.archetypeRow = destEntityRow;

        MecsVec<MecsComponentID> componentIDS;
        {
            const Archetype& sourceArch = world->archetypes.at(source->archetype);
            destinationWorld->newEvents.push(world->memAllocator, WorldEvent {
                                                                      .kind = WorldEventKind::eNewEntity,
                                                                      .entityID = newEntityID,
                                                                  });
            componentIDS.resize(world->memAllocator, sourceArch.componentIDs.count());
            for (MecsSize i = 0; i < sourceArch.componentIDs.count(); i++) {
                componentIDS[i] = sourceArch.componentIDs[i];
            }
        }
        BitSet tempBitSet;
        ArchetypeID oldArchetypeID = findArchetype(world, tempBitSet);
        componentIDS.forEach([&](MecsComponentID component) {
            const Archetype& sourceArch = world->archetypes.at(source->archetype);
            const Archetype& destArch = destinationWorld->archetypes.at(destArchetypeID);

            const void* sourceRow = sourceArch.storage.getRowComponent(component, source->archetypeRow);
            void* destRow = destArch.storage.getRowComponent(component, destEntity.archetypeRow);

            MecsComponentInfoInternal& info = registry->components.at(component);
            if (info.copy) {
                info.copy(sourceRow, destRow, info.size);
            } else {
                mecsMemCpy(static_cast<const char*>(sourceRow), info.size, static_cast<char*>(destRow), info.size);
            }
            tempBitSet.set(world->memAllocator, component, true);

            ArchetypeID newArchetypeID = findArchetype(world, tempBitSet);
            destinationWorld->newEvents.push(world->memAllocator, WorldEvent {
                                                                      .kind = WorldEventKind::eNewComponent,
                                                                      .entityID = newEntityID,
                                                                      .componentID = component,
                                                                      .archetypeID = oldArchetypeID,
                                                                      .newArchetypeID = newArchetypeID,
                                                                  });
            oldArchetypeID = newArchetypeID;
        });
        tempBitSet.destroy(world->memAllocator);
        componentIDS.destroy(world->memAllocator);

        *destinationWorld->entities.at(newEntityID) = destEntity;

        return newEntityID;
    }
}

void* mecsWorldAddComponent(MecsWorld* const world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(entity != MECS_INVALID && "Invalid entity ID");
    MecsEntity* ent = world->entities.at(entity);
    MECS_ASSERT(ent->status != EntityStatus::eDestroying);
    MecsComponentInfoInternal info = world->registry->components[component];

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
                                                       .archetypeID = MECS_INVALID,
                                                   });
    } else {
        ArchetypeID oldArchetypeID = ent->archetype;
        Archetype& oldArchetype = world->archetypes[oldArchetypeID];

        if (oldArchetype.storage.hasComponent(component)) {
            // We're re-adding an existing component
            outPtr = oldArchetype.storage.getRowComponent(component, ent->archetypeRow);
            if (info.destroy != nullptr) { info.destroy(outPtr); }
            world->newEvents.push(world->memAllocator, WorldEvent {
                                                           .kind = WorldEventKind::eUpdateComponent,
                                                           .entityID = entity,
                                                           .componentID = component,
                                                           .archetypeID = oldArchetypeID,
                                                            .newArchetypeID = oldArchetypeID,
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
                                                           .archetypeID = oldArchetypeID,
                                                           .newArchetypeID = newArchetypeID,
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
    MecsComponentInfoInternal& info = world->registry->components[component];
    const Archetype& archetype = world->archetypes[ent->archetype];
    return archetype.storage.hasComponent(component);
}
void* mecsWorldEntityGetComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(entity != MECS_INVALID && "Invalid entity ID");

    MecsEntity* ent = world->entities.at(entity);
    MecsComponentInfoInternal& info = world->registry->components[component];
    const Archetype& archetype = world->archetypes[ent->archetype];
    MECS_ASSERT(archetype.storage.hasComponent(component));
    return archetype.storage.getRowComponent(component, ent->archetypeRow);
}
void mecsWorldRemoveComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    MECS_ASSERT(mecsWorldEntityHasComponent(world, entity, component));

    // Component removal is deferred to the next flushUpdates
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eDestroyComponent,
                                                   .entityID = entity,
                                                   .componentID = component,
                                               });
}
MecsSize mecsWorldEntityGetNumComponents(MecsWorld* world, MecsEntityID entity)
{
    MECS_ASSERT(world && world->registry);

    MecsEntity* ent = world->entities.at(entity);
    const Archetype& archetype = world->archetypes[ent->archetype];
    return archetype.componentIDs.count();
}
MecsComponentID mecsWorldEntityGetComponentByIndex(MecsWorld* world, MecsEntityID entity, MecsSize index)
{
    MecsEntity* ent = world->entities.at(entity);
    const Archetype& archetype = world->archetypes[ent->archetype];
    if (!archetype.componentIDs.isValid(index)) { return MECS_INVALID; }
    return archetype.componentIDs[index];
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

void mecsWorldEntityChanged(MecsWorld* world, MecsEntityID entityID)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid entity ID");
    if (ent->status == EntityStatus::eDestroying) { return; }
    world->newEvents.push(world->memAllocator, WorldEvent { .kind = WorldEventKind::eRecreateEntity, .entityID = entityID });
}

void mecsOnEntityRecreate(MecsWorld* world, MecsEntityID entityID, void* updateData)
{

    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid entity ID");
    if (ent->status == EntityStatus::eDestroying) { return; }

    MecsRegistry* registry = world->registry;
    MECS_ASSERT(registry);

    const Archetype& arch = world->archetypes[ent->archetype];
    arch.componentIDs.forEach([&](MecsComponentID component) {
        void* pComponent = arch.storage.getRowComponent(component, ent->archetypeRow);
        const ComponentInfo& info = registry->components[component];
        if (info.teardown != nullptr) {
            info.teardown(world, entityID, pComponent, updateData);
        }
    });

    arch.componentIDs.forEach([&](MecsComponentID component) {
        void* pComponent = arch.storage.getRowComponent(component, ent->archetypeRow);
        const ComponentInfo& info = registry->components[component];
        if (info.setup != nullptr) {
            info.setup(world, entityID, pComponent, updateData);
        }
    });
}
void mecsWorldFlushEvents(MecsWorld* world, void* updateData)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    world->newEvents.forEach([&](const WorldEvent& event) {
        switch (event.kind) {
        case WorldEventKind::eNewEntity: {
            mecsOnNewEntitySpawned(world, event.entityID, updateData);
            break;
        }
        case WorldEventKind::eDestroyEntity: {
            mecsOnEntityDestroyed(world, event.entityID, updateData);
            break;
        }
        case WorldEventKind::eRecreateEntity: {
            mecsOnEntityRecreate(world, event.entityID, updateData);
            break;
        }
        case WorldEventKind::eNewComponent:
        case WorldEventKind::eUpdateComponent: {
            mecsOnComponentAddedToEntity(world, event.entityID, event.componentID, event.archetypeID, event.newArchetypeID, updateData);
            break;
        }
        case WorldEventKind::eDestroyComponent: {
            mecsOnComponentRemovedFromEntity(world, event.entityID, event.componentID, updateData);
            break;
        }
        }
    });
    world->newEvents.clear();

    mecsWorldRunSystems(world, updateData);
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

MecsSystemID mecsWorldDefineSystem(MecsWorld* world, const MecsDefineSystemInfo* systemInfo)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MECS_ASSERT(systemInfo != nullptr && "systemInfo must not be null");
    MECS_ASSERT(systemInfo->systemRun != nullptr && "systemInfo->systemRun must not be null");
    if (systemInfo->numComponents > 0) {
        MECS_ASSERT(systemInfo->pComponents != nullptr && "if systemInfo->numComponents is not 0, systemInfo->pComponents must not be null");
        MECS_ASSERT(systemInfo->pFilters != nullptr && "if systemInfo->numComponents is not 0, systemInfo->pFilters must not be null");
    }

    MecsSystem system {};
    system.systemFlags = systemInfo->systemFlags;
    system.systemData = systemInfo->systemData;
    system.onEntityAdded = systemInfo->onEntityAdded;
    system.systemRun = systemInfo->systemRun;
    system.onEntityRemoved = systemInfo->onEntityRemoved;

    system.systemIterator = mecsWorldAcquireIterator(world);
    BitSet systemArchetypeBitset;

    for (MecsU32 i = 0; i < systemInfo->numComponents; i++) {
        MecsComponentID component = systemInfo->pComponents[i];
        if (component == MECS_INVALID) { continue; }
        MecsIteratorFilter filter = systemInfo->pFilters[i];
        mecsIterComponentFilter(system.systemIterator, component, filter, i);

        if (filter == With || filter == Access) {
            systemArchetypeBitset.set(world->memAllocator, component, true);
        }
    }
    mecsIteratorFinalize(system.systemIterator);
    system.systemArchetype = findArchetype(world, systemArchetypeBitset);
    systemArchetypeBitset.destroy(world->memAllocator);

    MecsSystemID systemID = world->systems.push(world->memAllocator, std::move(system));
    return systemID;
}

MecsRegistry* mecsWorldGetRegistry(MecsWorld* world)
{
    MECS_ASSERT(world != nullptr && "World must not be null");
    return world->registry;
}

void mecsWorldRunSystems(MecsWorld* world, void* updateData)
{
    world->systems.forEach([updateData](MecsSystem& system) {
        MECS_ASSERT(system.systemRun);
        mecsIteratorBegin(system.systemIterator);
        system.systemRun(system.systemData, updateData, system.systemIterator);
    });
}
