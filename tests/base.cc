#include "mecs/base.h"
#include "catch_amalgamated.hpp"
#include "mecs/iterator.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

#include "mecshpp/mecs.hpp"
#include "test_private.hpp"
#include <vector>
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

TEST_CASE("C++ resource management")
{
    static MecsSize gNumConstructorCalls = 0;
    static MecsSize gNumDestructorCalls = 0;
    {
        struct Name {
            std::string name;
        };

        struct VectorNames {
            std::vector<std::string> names;
        };

        struct ConstructorDestructor {
            ConstructorDestructor()
            {
                gNumConstructorCalls++;
            }
            ConstructorDestructor(const ConstructorDestructor&) {

            };
            ~ConstructorDestructor()
            {
                gNumDestructorCalls++;
            }
        };

        mecs::Registry registry(MecsRegistryCreateInfo { .memAllocator = kDebugAllocator });
        registry.addRegistration<Name>("Name");
        registry.addRegistration<VectorNames>("VectorNames");
        registry.addRegistration<ConstructorDestructor>("ConstructorDestructor");

        mecs::PrefabID baseName = registry.createPrefab().withComponent<Name>("Mayo");
        mecs::PrefabID vectorOfNames = registry.createPrefab().withComponent<VectorNames>(VectorNames({ "Foo", "Bar", "Baz" }));
        mecs::PrefabID constructorDestructor = registry.createPrefab().withComponent<ConstructorDestructor>();

        // Two constructors should have been called
        // One for the default-initialized value
        // One for the explicit default value that was passed to withComponent
        // The destructor should have been called once for the explicit default value
        REQUIRE(gNumConstructorCalls == 2);
        REQUIRE(gNumDestructorCalls == 1);

        mecs::World world(registry);
        mecs::EntityID entity = world.spawnEntityPrefab(baseName);

        mecs::EntityID entity2 = world.spawnEntityPrefab(constructorDestructor);
        REQUIRE(gNumConstructorCalls == 3);
        REQUIRE(gNumDestructorCalls == 1);
        world.entityAddComponent<Name>(entity2);

        // Constructor for the new row should have been called (+1 c)
        // Copy constructor should have been called from old row to new row
        // Destructor should have been called on old row
        REQUIRE(gNumConstructorCalls == 4);
        REQUIRE(gNumDestructorCalls == 2);
        world.destroyEntity(entity2);
        world.flushEvents();

        // Destructor should have been called on row
        REQUIRE(gNumConstructorCalls == 4);
        REQUIRE(gNumDestructorCalls == 3);

        {
            Name& n = world.entityGetComponent<Name>(entity);
            REQUIRE(n.name == "Mayo");
            n.name = "Tizio";
        }
        world.entityAddComponent<VectorNames>(entity);
        {
            // Need another pointer because the entity was moved to another storage
            Name& n = world.entityGetComponent<Name>(entity);
            REQUIRE(n.name == "Tizio");

            Name* pb = registry.getPrefabComponent<Name>(baseName);
            REQUIRE(pb->name == "Mayo");

            VectorNames& vn = world.entityGetComponent<VectorNames>(entity);
            REQUIRE(vn.names.empty());
        }
    }
    // No further constructors should have been called
    REQUIRE(gNumConstructorCalls == 4);
    // Prefab destructor should have been called now
    REQUIRE(gNumDestructorCalls == 4);
}

TEST_CASE("Order of operations")
{
    mecs::Registry reg({ kDebugAllocator });

    struct Foo { };
    struct Bar { };
    reg.addRegistration<Foo>("Foo");
    reg.addRegistration<Bar>("Bar");

    mecs::World world(reg);
    mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
    mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

    mecs::EntityID entity = world.spawnEntity()
                                .withComponent<Foo>();

    // Entities aren't spawned until flushEvents
    REQUIRE(mecs::utils::count(fooIterator) == 0);
    world.flushEvents();

    REQUIRE(mecs::utils::count(fooIterator) == 1);

    // A new archetype <Foo, Bar> is created
    world.entityAddComponent<Bar>(entity);
    // Archetype changes are instantaneous
    REQUIRE(mecs::utils::count(fooIterator) == 0);
    // But aqcuired iterators are only updated during flushEvents()
    REQUIRE(mecs::utils::count(barIterator) == 0);
    world.flushEvents();
    REQUIRE(mecs::utils::count(barIterator) == 1);

    world.destroyEntity(entity);
    // Entities aren't destroyed until flushEvents
    REQUIRE(mecs::utils::count(barIterator) == 1);
    world.flushEvents();
    // Entities aren't destroyed until flushEvents
    REQUIRE(mecs::utils::count(barIterator) == 0);
}

TEST_CASE("Iteration filters")
{
    mecs::Registry reg({ kDebugAllocator });

    struct Foo {
        int x;
    };
    struct Bar {
        int y { 2 };
    };
    struct Baz { };
    struct ID {
        int id;
    };
    reg.addRegistration<Foo>("Foo");
    reg.addRegistration<Bar>("Bar");
    reg.addRegistration<Baz>("Baz");
    reg.addRegistration<ID>("ID");

    mecs::World world(reg);
    mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
    mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

    mecs::EntityID ent0 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<ID>(0);
    mecs::EntityID ent1 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<Bar>()
                              .withComponent<ID>(1);
    mecs::EntityID ent2 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<Bar>()
                              .withComponent<Baz>()
                              .withComponent<ID>(2);
    mecs::EntityID ent3 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<Baz>()
                              .withComponent<ID>(3);

    {
        // Iterate all components with a Foo
        mecs::Iterator allFoos = world.acquireIterator<const Foo&>();
        REQUIRE(mecs::utils::count(allFoos) == 4);
    }

    {
        // Iterate all components with a Foo that do not have a Bar
        mecs::Iterator allFoosNoBar = world.acquireIterator<const Foo&, mecs::Not<Bar>>();
        REQUIRE(mecs::utils::count(allFoosNoBar) == 2);
    }

    {
        // Iterate all components with a Foo, a Bar that do not have a Baz
        mecs::Iterator allFooBazNoBar = world.acquireIterator<Foo&, const Bar&, mecs::Not<Baz>>();
        REQUIRE(mecs::utils::count(allFooBazNoBar) == 1);
        allFooBazNoBar.forEach([&](Foo& foo, const Bar& bar, mecs::Not<Baz>) {
            foo.x += bar.y;
        });

        REQUIRE(world.entityGetComponent<Foo>(ent0).x == 0);
        REQUIRE(world.entityGetComponent<Foo>(ent1).x == 2);
        REQUIRE(world.entityGetComponent<Foo>(ent2).x == 0);
        REQUIRE(world.entityGetComponent<Foo>(ent3).x == 0);
    }
}
TEST_CASE("Iteration filters 2")
{
    mecs::Registry reg({ kDebugAllocator });

    struct Foo {
        int x;
    };
    struct Bar {
        int y { 2 };
    };
    struct Baz { };
    struct ID {
        int id;
    };
    reg.addRegistration<Foo>("Foo");
    reg.addRegistration<Bar>("Bar");
    reg.addRegistration<Baz>("Baz");
    reg.addRegistration<ID>("ID");

    mecs::World world(reg);
    mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
    mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

    mecs::EntityID ent0 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<Baz>()
                              .withComponent<ID>(0);
    mecs::EntityID ent1 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<Bar>()
                              .withComponent<ID>(1);
    mecs::EntityID ent2 = world.spawnEntity()
                              .withComponent<Foo>()
                              .withComponent<ID>(0);

    {
        // Iterate all components with a Foo and a Bar
        mecs::Iterator allFoos = world.acquireIterator<const Foo&, const Baz&>();
        REQUIRE(mecs::utils::count(allFoos) == 1);
    }
    {
        // Iterate all components with a Foo, not a Baz
        mecs::Iterator allFoos = world.acquireIterator<const Foo&, mecs::Not<Baz>>();
        REQUIRE(mecs::utils::count(allFoos) == 2);
    }
}

// NOLINTEND this is a test file