#include "mecs/base.h"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

#include "test_private.hpp"
// NOLINTBEGIN this is a test file

TEST_CASE("Basics")
{
    struct Foo {
    };

    struct Bar {
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, Foo);
    MECS_REGISTER_COMPONENT(registry, Bar);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    MecsEntityID ent0 = mecsWorldSpawnEntity(world, nullptr);
    MecsEntityID ent1 = mecsWorldSpawnEntity(world, nullptr);
    MecsEntityID ent2 = mecsWorldSpawnEntity(world, nullptr);
    MecsEntityID ent3 = mecsWorldSpawnEntity(world, nullptr);

    MECS_COMPONENT(world, ent0, Foo);
    MECS_COMPONENT(world, ent1, Bar);
    MECS_COMPONENT(world, ent2, Foo);
    MECS_COMPONENT(world, ent2, Bar);

    mecsWorldFlushEvents(world);

    {
        MecsIterator* iterator = mecsWorldAcquireIterator(world);
        mecsIterComponent(iterator, Component_Foo, 0);
        mecsIteratorFinalize(iterator);

        mecsIteratorBegin(iterator);
        MecsU32 foos = 0;
        while (mecsIteratorAdvance(iterator)) {
            foos++;
        }

        REQUIRE(foos == 2);
        mecsWorldReleaseIterator(world, iterator);
    }
    {
        MecsIterator* iterator = mecsWorldAcquireIterator(world);
        mecsIterComponent(iterator, Component_Bar, 0);
        mecsIteratorFinalize(iterator);

        mecsIteratorBegin(iterator);
        MecsU32 bars = 0;
        while (mecsIteratorAdvance(iterator)) {
            bars++;
        }

        REQUIRE(bars == 2);
        mecsWorldReleaseIterator(world, iterator);
    }

    {
        MecsIterator* iterator = mecsWorldAcquireIterator(world);
        mecsIterComponent(iterator, Component_Foo, 0);
        mecsIterComponent(iterator, Component_Bar, 1);
        mecsIteratorFinalize(iterator);

        mecsIteratorBegin(iterator);
        MecsU32 fooobars = 0;
        while (mecsIteratorAdvance(iterator)) {
            fooobars++;
        }

        REQUIRE(fooobars == 1);
        mecsWorldReleaseIterator(world, iterator);
    }
    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

TEST_CASE("Iterator automatic updates")
{
    struct Foo {
    };

    struct Bar {
    };

    struct Baz {
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, Foo);
    MECS_REGISTER_COMPONENT(registry, Bar);
    MECS_REGISTER_COMPONENT(registry, Baz);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    MecsEntityID ent0 = mecsWorldSpawnEntity(world, nullptr);
    MECS_COMPONENT(world, ent0, Foo);
    MECS_COMPONENT(world, ent0, Bar);
    MECS_COMPONENT(world, ent0, Baz);
    MecsEntityID ent1 = mecsWorldSpawnEntity(world, nullptr);
    MECS_COMPONENT(world, ent1, Foo);

    mecsWorldFlushEvents(world);
    MecsIterator* iterator = mecsWorldAcquireIterator(world);
    mecsIterComponent(iterator, Component_Foo, 0);
    mecsIterComponent(iterator, Component_Bar, 1);
    mecsIterComponent(iterator, Component_Baz, 2);
    mecsIteratorFinalize(iterator);

    {
        mecsIteratorBegin(iterator);
        MecsU32 foos = 0;
        while (mecsIteratorAdvance(iterator)) {
            foos++;
        }

        REQUIRE(foos == 1);
    }

    MECS_COMPONENT(world, ent1, Bar);
    mecsWorldFlushEvents(world);

    {
        mecsIteratorBegin(iterator);
        MecsU32 foos = 0;
        while (mecsIteratorAdvance(iterator)) {
            foos++;
        }

        // Still 1 foo because the iterator requires for each entity a Foo, a Bar, a Baz
        REQUIRE(foos == 1);
    }
    MECS_COMPONENT(world, ent1, Baz);

    mecsWorldFlushEvents(world);
    {
        mecsIteratorBegin(iterator);
        MecsU32 foos = 0;
        while (mecsIteratorAdvance(iterator)) {
            foos++;
        }

        REQUIRE(foos == 2);
    }
    mecsWorldReleaseIterator(world, iterator);
    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

TEST_CASE("Simple loop")
{
    struct Counter {
        int count;
    };
    struct Bar {
        int count;
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, Counter);
    MECS_REGISTER_COMPONENT(registry, Bar);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    MecsEntityID ent0 = mecsWorldSpawnEntity(world, {});
    mecsWorldAddComponent(world, ent0, Component_Counter);

    mecsWorldFlushEvents(world);

    MecsIterator* iterator = mecsWorldAcquireIterator(world);
    mecsIterComponent(iterator, Component_Counter, 0);
    mecsIteratorFinalize(iterator);

    for (int i = 0; i < 100; i++) { /// NOLINT
        mecsIteratorBegin(iterator);
        int numItems = 0;
        while (mecsIteratorAdvance(iterator)) {
            Counter* cnt = static_cast<Counter*>(mecsIteratorGetArgument(iterator, 0));
            cnt->count += 1;
            numItems += 1;
        }

        REQUIRE(numItems == 1);
    }
    {
        Counter* cnt = static_cast<Counter*>(mecsWorldEntityGetComponent(world, ent0, Component_Counter));
        REQUIRE(cnt->count == 100);
    }
    mecsWorldAddComponent(world, ent0, Component_Bar);
    mecsWorldFlushEvents(world);
    {
        Counter* cnt = static_cast<Counter*>(mecsWorldEntityGetComponent(world, ent0, Component_Counter));
        REQUIRE(cnt->count == 100);
    }

    for (int i = 0; i < 100; i++) { /// NOLINT
        mecsIteratorBegin(iterator);
        int numItems = 0;
        while (mecsIteratorAdvance(iterator)) {
            Counter* cnt = static_cast<Counter*>(mecsIteratorGetArgument(iterator, 0));
            cnt->count += 1;
            numItems += 1;
        }
        REQUIRE(numItems == 1);
    }
    {
        Counter* cnt = static_cast<Counter*>(mecsWorldEntityGetComponent(world, ent0, Component_Counter));
        REQUIRE(cnt->count == 200);
    }
    mecsWorldRemoveComponent(world, ent0, Component_Bar);
    mecsWorldFlushEvents(world);

    for (int i = 0; i < 100; i++) { /// NOLINT
        mecsIteratorBegin(iterator);
        int numItems = 0;
        while (mecsIteratorAdvance(iterator)) {
            Counter* cnt = static_cast<Counter*>(mecsIteratorGetArgument(iterator, 0));
            cnt->count += 1;
            numItems += 1;
        }
        REQUIRE(numItems == 1);
    }
    {
        Counter* cnt = static_cast<Counter*>(mecsWorldEntityGetComponent(world, ent0, Component_Counter));
        REQUIRE(cnt->count == 300);
    }
    mecsWorldReleaseIterator(world, iterator);
    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

TEST_CASE("Spawning components in a loop")
{

    struct BulletSpawner {
        int numBullets;
    };
    struct Bullet {
        int health;
    };

    struct Position {
        float x, y, z;
    };

    struct Velocity {
        float x, y, z;
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, BulletSpawner);
    MECS_REGISTER_COMPONENT(registry, Bullet);
    MECS_REGISTER_COMPONENT(registry, Position);
    MECS_REGISTER_COMPONENT(registry, Velocity);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);
    MecsEntityID bulletSpawner = mecsWorldSpawnEntity(world, {});
    BulletSpawner* bulletSpawnerComponent = (BulletSpawner*)mecsWorldAddComponent(world, bulletSpawner, Component_BulletSpawner);
    bulletSpawnerComponent->numBullets = 100;

    /// Bullet spawner
    MecsIterator* bulletSpawnIterator = mecsWorldAcquireIterator(world);
    mecsIterComponent(bulletSpawnIterator, Component_BulletSpawner, 0);
    mecsIteratorFinalize(bulletSpawnIterator);

    /// Bullet mover
    MecsIterator* bulletMoverIterator = mecsWorldAcquireIterator(world);
    mecsIterComponent(bulletMoverIterator, Component_Bullet, 0);
    mecsIterComponent(bulletMoverIterator, Component_Position, 1);
    mecsIterComponent(bulletMoverIterator, Component_Velocity, 2);
    mecsIteratorFinalize(bulletMoverIterator);

    /// Bullet lifetime
    MecsIterator* bulletLifetimeIterator = mecsWorldAcquireIterator(world);
    mecsIterComponent(bulletLifetimeIterator, Component_Bullet, 0);
    mecsIteratorFinalize(bulletLifetimeIterator);

    static MecsSize numSpawnedBullets = 0;
    static MecsSize numDestroyedBullets = 0;

    constexpr MecsSize kNumIterations = 10000;
    for (MecsSize i = 0; i < kNumIterations; i++) {
        {
            mecsIteratorBegin(bulletSpawnIterator);
            constexpr MecsSize kNumBulletsToSpawnEachIteration = 10;
            while (mecsIteratorAdvance(bulletSpawnIterator)) {
                BulletSpawner* bulletSpawner = (BulletSpawner*)mecsIteratorGetArgument(bulletSpawnIterator, 0);
                if (bulletSpawner->numBullets == 0) {
                    break;
                }

                for (MecsSize b = 0; b < kNumBulletsToSpawnEachIteration; b++) {
                    MecsEntityID bullet = mecsWorldSpawnEntity(world, nullptr);
                    {
                        Bullet* bullComp = (Bullet*)mecsWorldAddComponent(world, bullet, Component_Bullet);
                        bullComp->health = 10;
                    };
                    {
                        Position* pos = (Position*)mecsWorldAddComponent(world, bullet, Component_Position);
                        pos->x = pos->y = pos->z = 0.0f;
                    }

                    {
                        Velocity* vel = (Velocity*)mecsWorldAddComponent(world, bullet, Component_Velocity);
                        vel->x = vel->y = vel->z = 0.0f;
                        vel->y = 10.0f;
                    }
                    numSpawnedBullets++;
                }
                bulletSpawner->numBullets -= kNumBulletsToSpawnEachIteration;
            }
        }
        {
            mecsIteratorBegin(bulletMoverIterator);
            while (mecsIteratorAdvance(bulletMoverIterator)) {
                Position* pos = (Position*)mecsIteratorGetArgument(bulletMoverIterator, 1);
                Velocity* vel = (Velocity*)mecsIteratorGetArgument(bulletMoverIterator, 2);
                pos->x += vel->x;
                pos->y += vel->y;
                pos->z += vel->z;
            }
        }
        {
            mecsIteratorBegin(bulletLifetimeIterator);
            while (mecsIteratorAdvance(bulletLifetimeIterator)) {
                Bullet* pos = (Bullet*)mecsIteratorGetArgument(bulletLifetimeIterator, 0);
                pos->health -= 1;
                if (pos->health == 0) {
                    MecsEntityID entity = mecsIteratorGetEntity(bulletLifetimeIterator);
                    mecsWorldDestroyEntity(world, entity);
                    numDestroyedBullets++;
                }
            }
        }

        mecsWorldFlushEvents(world);
    }

    REQUIRE(numSpawnedBullets == 100);
    REQUIRE(numDestroyedBullets == 100);

    mecsWorldReleaseIterator(world, bulletLifetimeIterator);
    mecsWorldReleaseIterator(world, bulletMoverIterator);
    mecsWorldReleaseIterator(world, bulletSpawnIterator);

    {
        // Count the number of bullets: there should be none
        MecsIterator* bulletCounter = mecsWorldAcquireIterator(world);
        mecsIterComponent(bulletCounter, Component_Bullet, 0);
        mecsIteratorFinalize(bulletCounter);

        MecsSize numBullets = 0;
        mecsIteratorBegin(bulletCounter);
        while (mecsIteratorAdvance(bulletCounter)) {
            numBullets++;
        }
        REQUIRE(numBullets == 0);
        mecsWorldReleaseIterator(world, bulletCounter);
    }

    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

// NOLINTEND this is a test file