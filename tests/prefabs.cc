#include "mecs/base.h"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

#include "test_private.hpp"
// NOLINTBEGIN this is a test file

TEST_CASE("Prefabs")
{
    struct Position {
        int x, y, z;
    };

    struct Velocity {
        int x, y, z;
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, Position);
    MECS_REGISTER_COMPONENT(registry, Velocity);

    /* Define a prefab that has a Position component and a Velocity component
       The Position component will be zero-initialized, while the Velocity component is given an explicit default value
    */
    MecsPrefabID prefab = mecsRegistryCreatePrefab(registry);
    Velocity velocityDefault { 10, 54, 0 };
    mecsRegistryPrefabAddComponent(registry, prefab, Component_Position);
    mecsRegistryPrefabAddComponentWithDefaults(registry, prefab, Component_Velocity, &velocityDefault);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    // Now spawn the prefab: this entity will have a Position component and a Velocity, with it's values copied from the prefab
    MecsEntityID ent0 = mecsWorldSpawnEntityPrefab(world, prefab, nullptr);

    mecsWorldFlushEvents(world);

    {
        MecsIterator* iterator = mecsWorldAcquireIterator(world);
        mecsIterComponent(iterator, Component_Position, 0);
        mecsIterComponent(iterator, Component_Velocity, 1);
        mecsIteratorFinalize(iterator);

        mecsIteratorBegin(iterator);
        MecsU32 foos = 0;
        while (mecsIteratorAdvance(iterator)) {
            foos++;
            Position* pos = static_cast<Position*>(mecsIteratorGetArgument(iterator, 0));
            Velocity* vel = static_cast<Velocity*>(mecsIteratorGetArgument(iterator, 1));
            REQUIRE(pos->x == 0);
            REQUIRE(pos->y == 0);
            REQUIRE(pos->z == 0);
            REQUIRE(vel->x == 10);
            REQUIRE(vel->y == 54);
            REQUIRE(vel->z == 0);
        }

        REQUIRE(foos == 1);
        mecsWorldReleaseIterator(world, iterator);
    }

    mecsWorldFree(world);
    mecsRegistryFree(registry);
}
// NOLINTEND