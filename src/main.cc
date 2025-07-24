#include "mecs/base.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"
#include <print>

struct Position {
    float x;
    float y;
    float z;
};

struct Velocity {
    float x;
    float y;
    float z;
};

int main(int argc, char** argv)
{
    /* Holds all global registrations */
    MecsRegistry* registry = mecsRegistryCreate(nullptr);
    ComponentInfo positionInfo = MECS_COMPONENTINFO(Position);
    MecsComponentID positionComponent = mecsRegistryAddRegistration(registry, &positionInfo);

    ComponentInfo velocityInfo = MECS_COMPONENTINFO(Velocity);
    MecsComponentID velocityComponent = mecsRegistryAddRegistration(registry, &velocityInfo);

    /* Holds all entities + their components */
    MecsWorldCreateInfo worldInfo;
    worldInfo.registry = registry;
    worldInfo.memAllocator = {};
    MecsWorld* world = mecsWorldCreate(&worldInfo);

    MecsEntityID entity0 = mecsWorldSpawnEntity(world, nullptr);
    mecsWorldAddComponent(world, entity0, positionComponent);

    MecsEntityID entity1 = mecsWorldSpawnEntity(world, nullptr);
    mecsWorldAddComponent(world, entity1, positionComponent);
    mecsWorldAddComponent(world, entity1, velocityComponent);

    // Entities aren't spawned (or destroyed) until the next call to worldFlushEvents
    mecsWorldFlushEvent(world);

    // An iterator is used to iter over all entities matching certain properties
    MecsIterator* iterator = mecsWorldAcquireIterator(world);

    // First you describe the iterator arguments
    mecsIterComponent(iterator, positionComponent, 0);
    mecsIterComponent(iterator, velocityComponent, 1);

    // Then you finalize it
    mecsIteratorFinalize(iterator);

    constexpr MecsSize kMaxIterations = 10;
    for (MecsSize iterations = 0; iterations < kMaxIterations; iterations++) {

        mecsIteratorBegin(iterator);
        while (mecsIteratorAdvance(iterator)) {
            auto* pos = static_cast<Position*>(mecsIteratorGetArgument(iterator, 0));
            auto* vel = static_cast<Velocity*>(mecsIteratorGetArgument(iterator, 1));
            pos->x += vel->x;
            pos->y += vel->y;
            pos->z += vel->z;

            constexpr float kGravity = 9.8F;
            vel->y += kGravity;
        }

        // An iterator can be started more than once
        // The idea is that you acquire it once, and then whenever you need to iterate over it
        // just call beginIterator
        mecsIteratorBegin(iterator);
        while (mecsIteratorAdvance(iterator)) {
            const auto* pos = static_cast<const Position*>(mecsIteratorGetArgument(iterator, 0));
            const auto* vel = static_cast<const Velocity*>(mecsIteratorGetArgument(iterator, 1));

            std::println("At iteration {}", iterations);
            std::println("Postion {} {} {}", pos->x, pos->y, pos->z);
            std::println("Velocity {} {} {}", vel->x, vel->y, vel->z);
        }
    }

    // // Remember to release the iterator when you don't need it anymore
    mecsWorldReleaseIterator(world, iterator);

    mecsWorldFree(world);
    mecsRegistryFree(registry);

    return 0;
}