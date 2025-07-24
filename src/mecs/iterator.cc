#include "mecs/base.h"

#include "private.h"

void mecsIterComponent(MecsIterator* iterator, MecsComponentID component, MecsSize argIndex)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eInitializing && "Cannot change the arguments of an iterator once it has begun for the first time");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);

    if (world->componentBuckets.isValid(component)) {
        ComponentBucket& bucket = world->componentBuckets.at(component);
        bucket.usedByIterators.pushUnique(world->memAllocator, iterator);
    }

    iterator->arguments.ensureSize(world->memAllocator, argIndex + 1);
    MecsIteratorArgument& argument = iterator->arguments[argIndex];
    argument.argumentID = component;
    argument.argumentType = ArgumentType::eComponent;
    argument.argumentAccess = ArgumentAccess::eInOut;
    iterator->dirty = true;
}
void mecsIteratorFinalize(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eInitializing && "Cannot finalize an iterator twice! Release it and acquire a new one");

    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);

    MecsSize numArgs = iterator->arguments.count();
    world->entities.forEach([&](
                                MecsEntityID ent,
                                const MecsEntity& entity) {
        for (MecsSize i = 0; i < numArgs; i++) {
            const MecsIteratorArgument& arg = iterator->arguments[i];
            if (arg.argumentType != ArgumentType::eComponent) {
                MECS_ASSERT(false && "TODO");
            }

            if (entity.status != EntityStatus::eSpawned) {
                // Don't iterate on entities that are either newly born or just killed
                return;
            }

            if (!mecsWorldEntityHasComponent(world, ent, arg.argumentID)) {
                return;
            }
        }

        iterator->entities.push(world->memAllocator, ent);
    });

    iterator->status = IteratorStatus::eIterating;
}
void mecsIteratorBegin(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    iterator->currentEntity = 0;
}
bool mecsIteratorAdvance(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->status == IteratorStatus::eIterating && "Cannot advance an iterator that hasn't begun");

    iterator->currentEntity += 1;

    if (iterator->entities.isValid(iterator->currentEntity - 1)) {
        iterator->currentEntityID = iterator->entities.at(iterator->currentEntity - 1);
    }

    return iterator->entities.isValid(iterator->currentEntity - 1);
}
void* mecsIteratorGetArgument(MecsIterator* iterator, MecsSize argIndex)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MECS_ASSERT(iterator->currentEntity > 0 && "Must have called mecsIteratorAdvance() at least once");
    MECS_ASSERT(iterator->status == IteratorStatus::eIterating && "Cannot fetch the argument of an iterator that hasn't begun");
    MECS_ASSERT(iterator->arguments.isValid(argIndex) && "Invalid argument index");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);

    MecsIteratorArgument& argument = iterator->arguments[argIndex];

    if (argument.argumentType == ArgumentType::eComponent) {
        MecsEntityID entityID = iterator->currentEntityID;
        ComponentStorage& storage = world->componentBuckets.at(argument.argumentID).storage;
        return storage.getComponent(entityID);
    }
    MECS_ASSERT("TODO");
    return nullptr;
}

MecsWorld* mecsIteratorGetWorld(MecsIterator* iterator)
{
    MECS_ASSERT(iterator != nullptr && "Cannot pass a null iterator");
    MecsWorld* world = iterator->world;
    MECS_ASSERT(world);
    return world;
}