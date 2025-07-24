#include "mecs/iterator.h"
#include "mecs/base.h"

#include "private.h"

void mecsIterComponent(MecsIterator* iterator, MecsComponentID component, MecsSize argIndex)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eInitializing && "Cannot change the arguments of an iterator once it has begun for the first time");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);

    iterator->componentSet.set(world->memAllocator, component, true);

    iterator->components.ensureSize(world->memAllocator, argIndex + 1);
    iterator->components[argIndex] = component;

    iterator->dirty = true;
}
void mecsIteratorFinalize(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eInitializing && "Cannot finalize an iterator twice! Release it and acquire a new one");

    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);

    MecsSize count = world->archetypes.count();

    iterator->status = IteratorStatus::eIterating;

    if (iterator->components.count() == 0) {
        // Iterator iterates on all entities
        iterator->archetypes.ensureSize(world->memAllocator, world->archetypes.count());
        for (MecsSize i = 0; i < count; i++) {
            const Archetype& archetype = world->archetypes[i];
            iterator->archetypes[i] = i;
        }
    } else {
        for (MecsSize i = 0; i < count; i++) {
            const Archetype& archetype = world->archetypes[i];
            if (iterator->componentSet.contains(archetype.storage.bitset())) {
                iterator->archetypes.push(world->memAllocator, i);
            }
        }
    }
}
void mecsIteratorBegin(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    iterator->currentArchetype = 0;
    iterator->currentRow = 0;
}
bool mecsIteratorAdvance(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eIterating && "Cannot advance an iterator that hasn't begun");

    if (iterator->archetypes.count() == 0) {
        return false;
    }

    MecsWorld* world = iterator->world;
    ArchetypeID worldArchetypeIndex = iterator->archetypes[iterator->currentArchetype];
    const Archetype& currentArchetype = world->archetypes[worldArchetypeIndex];
    if (iterator->currentRow >= currentArchetype.storage.rows()) {

        ArchetypeID worldArchetypeIndex = iterator->archetypes[iterator->currentArchetype];

        do {
            iterator->currentArchetype++;
            if (iterator->currentArchetype >= iterator->archetypes.count()) {
                return false;
            }
            worldArchetypeIndex = iterator->archetypes[iterator->currentArchetype];
        } while (world->archetypes[worldArchetypeIndex].storage.rows() == 0);
        iterator->currentRow = 0;
    }

    if (iterator->currentArchetype >= iterator->archetypes.count()) {
        return false;
    }

    iterator->currentRow += 1;
    return true;
}
void* mecsIteratorGetArgument(MecsIterator* iterator, MecsSize argIndex)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->currentRow > 0 && "Must have called mecsIteratorAdvance() at least once");
    MECS_ASSERT(iterator->status == IteratorStatus::eIterating && "Cannot fetch the argument of an iterator that hasn't begun");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    ArchetypeID worldArchetypeIndex = iterator->archetypes[iterator->currentArchetype];
    const Archetype& currentArchetype = world->archetypes[worldArchetypeIndex];
    MecsComponentID component = iterator->components[argIndex];
    return currentArchetype.storage.getRowComponent(component, iterator->currentRow - 1);
}

MecsWorld* mecsIteratorGetWorld(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    return world;
}
MecsEntityID mecsIteratorGetEntity(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    ArchetypeID worldArchetypeIndex = iterator->archetypes[iterator->currentArchetype];
    const Archetype& currentArchetype = world->archetypes[worldArchetypeIndex];
    MecsEntityID entity = currentArchetype.rowToEntity[iterator->currentRow - 1];
    return entity;
}

MECS_API MecsSize mecsUtilIteratorCount(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    mecsIteratorBegin(iterator);
    MecsSize count = 0;
    while (mecsIteratorAdvance(iterator)) {
        count++;
    }
    return count;
}