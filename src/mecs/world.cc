
#include "mecs/world.h"

#include "collections.h"
#include "mecs/base.h"
#include "private.h"

#include <cassert>

constexpr MecsSize kComponentInstanceIDNumBits = 24;
constexpr MecsSize kComponentInstanceMask = ((1 << kComponentInstanceIDNumBits) - 1);

MecsWorld*
mecsWorldCreate(const MecsWorldCreateInfo* const mecsWorldCreateInfo)
{
    assert(mecsWorldCreateInfo != nullptr);

    MecsAllocator allocator;
    if (mecsWorldCreateInfo->memAllocator.memAlloc != nullptr) {
        allocator = mecsWorldCreateInfo->memAllocator;
    } else {
        allocator = kDefaultAllocator;
    }

    MecsWorld* world = mecsAlloc<MecsWorld>(allocator);

    world->registry = mecsWorldCreateInfo->registry;
    world->memAllocator = allocator;

    return world;
}

void mecsIteratorReleaseResources(const MecsAllocator& alloc, MecsIterator* iter)
{
    iter->arguments.destroy(alloc);
    iter->entities.destroy(alloc);
    mecsFree(alloc, iter);
}

void mecsWorldFree(MecsWorld* world)
{
    if (world == nullptr) {
        return;
    }

    mecsWorldFlushEvent(world);

    world->componentBuckets.forEach([world](ComponentBucket& bucket) {
        bucket.storage.destroy(world->memAllocator);
        bucket.usedByIterators.destroy(world->memAllocator);
    });
    world->componentBuckets.destroy(world->memAllocator);
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
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity ent = {};
    if (entityInfo != nullptr) {
        if (ent.name != nullptr) {
            ent.name = mecsStrDup(world->memAllocator, ent.name);
        }
    }
    ent.status = EntityStatus::eNewlySpawned;

    MecsEntityID entityID = world->entities.push(world->memAllocator, ent);
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eNewEntity,
                                                   .entityID = entityID,
                                               });

    return entityID;
}

void* mecsWorldAddComponent(MecsWorld* const world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    if (component >= world->componentBuckets.count()) {
        world->componentBuckets.ensureSize(world->memAllocator, component + 1);
        ComponentInfo info = world->registry->components[component];
        ComponentBucket& bucket = world->componentBuckets[component];
        bucket.storage = ComponentStorage(world->memAllocator, ElementInfo { info.size, info.align });
        bucket.usedByIterators = {};
    }

    MecsEntity* ent = world->entities.at(entity);
    MECS_ASSERT(ent != nullptr);
    ComponentStorage& storage = world->componentBuckets[component].storage;
    ComponentInfo info = world->registry->components[component];

    void* outPtr = nullptr;
    if (storage.hasComponent(entity) && info.destroy != nullptr) {
        // reuse the same storage slot
        outPtr = storage.getComponent(entity);
        info.destroy(outPtr);
        world->newEvents.push(world->memAllocator, WorldEvent {
                                                       .kind = WorldEventKind::eUpdateComponent,
                                                       .entityID = entity,
                                                       .componentID = component,
                                                   });
    } else {
        storage.constructUninitialized(world->memAllocator, entity, &outPtr);
        world->newEvents.push(world->memAllocator, WorldEvent {
                                                       .kind = WorldEventKind::eNewComponent,
                                                       .entityID = entity,
                                                       .componentID = component,
                                                   });
    }
    if (info.init != nullptr) {
        info.init(outPtr);
    }

    return outPtr;
}

bool mecsWorldEntityHasComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    if (component >= world->componentBuckets.count()) {
        return false;
    }

    MecsEntity* ent = world->entities.at(entity);
    MECS_ASSERT(ent != nullptr);
    ComponentStorage& storage = world->componentBuckets[component].storage;
    ComponentInfo info = world->registry->components[component];

    return storage.hasComponent(entity);
}

void mecsWorldRemoveComponent(MecsWorld* world, MecsEntityID entity, MecsComponentID component)
{
    MECS_ASSERT(world && world->registry);
    ComponentBucket& bucket = *worldGetBucket(world, component);
    void* pComponent = bucket.storage.getComponent(entity);
    MECS_ASSERT(pComponent != nullptr);

    ComponentInfo info
        = world->registry->components[component];
    if (info.destroy != nullptr) {
        info.destroy(pComponent);
    }
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eDestroyComponent,
                                                   .entityID = entity,
                                                   .componentID = component,
                                               });
    bucket.storage.destroyInstance(world->memAllocator, entity, component);
}
void mecsWorldDestroyEntity(MecsWorld* const world, MecsEntityID entityID)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    MecsEntity* ent = world->entities.at(entityID);
    if (ent->status == EntityStatus::eDestroying) { return; }
    ent->status = EntityStatus::eDestroying;
    world->newEvents.push(world->memAllocator, WorldEvent {
                                                   .kind = WorldEventKind::eDestroyEntity,
                                                   .entityID = entityID,
                                               });
}

void mecsFinalizeSpawnEntity(MecsWorld* const& world, MecsEntityID entityID)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");

    ent->status = EntityStatus::eSpawned;
}

void mecsAddEntityToIterators(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID component)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");

    ComponentBucket* bucket = worldGetBucket(world, component);
    MECS_ASSERT(bucket != nullptr);
    bucket->usedByIterators.forEach([&](MecsIterator* iterator) {
        // Ok to call now: this is happening in flushEvents, where no iterators are running
        iterator->entities.push(world->memAllocator, entityID);
    });
}

void mecsRemoveEntityToIterators(MecsWorld* const& world, MecsEntityID entityID, MecsComponentID component)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");

    ComponentBucket* bucket = worldGetBucket(world, component);
    MECS_ASSERT(bucket != nullptr);
    bucket->usedByIterators.forEach([&](MecsIterator* iterator) {
        // Ok to call now: this is happening in flushEvents, where no iterators are running
        iterator->entities.remove(entityID);
    });
}

void mecsFinalizeDestroyEntity(MecsWorld* const& world, MecsEntityID& entityID)
{
    MecsEntity* ent = world->entities.at(entityID);
    MECS_ASSERT(ent != nullptr && "Invalid index passed to destroyEntity");
    mecsFree(world->memAllocator, ent->name);
    ent->components.forEach([&](MecsComponentID component) {
        if (auto* bucket = worldGetBucket(world, component)) {
            bucket->usedByIterators.forEach([&](MecsIterator* itr) {
                // OK to at this stage: destroyEntityImpl is called in worldFlushEvents
                itr->entities.remove(entityID);
            });
        }
    });
    ent->components.destroy(world->memAllocator);
    world->entities.remove(world->memAllocator, entityID);
}

void mecsWorldFlushEvent(MecsWorld* world)
{
    MECS_ASSERT(world != nullptr && "Cannot pass a null world");
    while (!world->newEvents.empty()) {
        WorldEvent event = world->newEvents.pop();
        switch (event.kind) {
        case WorldEventKind::eNewEntity: {
            mecsFinalizeSpawnEntity(world, event.entityID);
            break;
        }
        case WorldEventKind::eDestroyEntity: {
            mecsFinalizeDestroyEntity(world, event.entityID);
            break;
        }
        case WorldEventKind::eNewComponent: {
            mecsAddEntityToIterators(world, event.entityID, event.componentID);
            break;
        }
        case WorldEventKind::eUpdateComponent: {
            // For now do nothing
            break;
        }
        case WorldEventKind::eDestroyComponent: {
            mecsRemoveEntityToIterators(world, event.entityID, event.componentID);
            break;
        };
        }
    }
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

    iterator->arguments.forEach([&](const MecsIteratorArgument& arg) {
        if (arg.argumentType == ArgumentType::eComponent) {
            ComponentBucket* bucket = worldGetBucket(world, arg.argumentID);
            if (bucket == nullptr) { return; }
            bucket->usedByIterators.remove(iterator);
        }
    });

    iterator->arguments.clear();
    iterator->entities.clear();
    world->reusableIterators.push(world->memAllocator, iterator);
    bool removed = world->acquiredIterators.remove(iterator);
    MECS_ASSERT(removed);
}