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

    mecsWorldFlushEvents(world, nullptr);

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

    mecsWorldFlushEvents(world, nullptr);
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
    mecsWorldFlushEvents(world, nullptr);

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

    mecsWorldFlushEvents(world, nullptr);
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

    mecsWorldFlushEvents(world, nullptr);

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
    mecsWorldFlushEvents(world, nullptr);
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
    mecsWorldFlushEvents(world, nullptr);

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

        mecsWorldFlushEvents(world, nullptr);
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

struct SystemAB {
    int numEntities;
};

struct SystemACD {
    int numEntities;
};

void onEntityAdded_SystemAB(void* data, void* update, MecsEntityID entity)
{
    SystemAB* system = (SystemAB*)data;
    system->numEntities++;
}

void systemRun_SystemAB(void* data, void* update, MecsIterator* iterator)
{

}


void onEntityRemoved_SystemAB(void* data, void* update, MecsEntityID entity)
{
    SystemAB* system = (SystemAB*)data;
    system->numEntities--;
}
void onEntityAdded_SystemACD(void* data, void* update, MecsEntityID entity)
{
    SystemACD* system = (SystemACD*)data;
    system->numEntities++;
}

void systemRun_SystemACD(void* data, void* update, MecsIterator* iterator)
{
}

void onEntityRemoved_SystemACD(void* data, void* update, MecsEntityID entity)

{
    SystemACD* system = (SystemACD*)data;
    system->numEntities--;
}

struct ComponentA {
};
struct ComponentB {
};
struct ComponentC {
};
struct ComponentD {
};

struct Transform{};
struct Mesh {};

struct DirectionalLight{};
struct PointLight{};
struct SpotLight{};

MECS_RTTI_SIMPLE(ComponentA);
MECS_RTTI_SIMPLE(ComponentB);
MECS_RTTI_SIMPLE(ComponentC);
MECS_RTTI_SIMPLE(ComponentD);

MECS_RTTI_SIMPLE(Transform);
MECS_RTTI_SIMPLE(Mesh);

MECS_RTTI_SIMPLE(DirectionalLight);
MECS_RTTI_SIMPLE(PointLight);
MECS_RTTI_SIMPLE(SpotLight);

TEST_CASE("Systems")
{
    SECTION("C system API") {
        MecsRegistryCreateInfo regInfo {};
        regInfo.memAllocator = kDebugAllocator;
        MecsRegistry* registry = mecsRegistryCreate(&regInfo);
        MECS_REGISTER_COMPONENT(registry, ComponentA);
        MECS_REGISTER_COMPONENT(registry, ComponentB);
        MECS_REGISTER_COMPONENT(registry, ComponentC);
        MECS_REGISTER_COMPONENT(registry, ComponentD);

        MecsWorld* world = mecsWorldCreate(registry, nullptr);

        SystemAB abSystem;
        abSystem.numEntities = 0;
        SystemACD acdSystem;
        acdSystem.numEntities = 0;

        {
            MecsComponentID components[] = {Component_ComponentA, Component_ComponentB};
            MecsIteratorFilter filters[] = {MecsIteratorFilter::Access, MecsIteratorFilter::Access};
            MecsDefineSystemInfo systemInfo {};
            systemInfo.numComponents = 2;
            systemInfo.pComponents = components;
            systemInfo.pFilters = filters;
            systemInfo.onEntityAdded = onEntityAdded_SystemAB;
            systemInfo.systemRun = systemRun_SystemAB;
            systemInfo.onEntityRemoved = onEntityRemoved_SystemAB;
            systemInfo.systemData = &abSystem;
            MecsSystemID abSystemID = mecsWorldDefineSystem(world, &systemInfo);
        }

        {
            MecsComponentID components[] = {Component_ComponentA, Component_ComponentC, Component_ComponentD};
            MecsIteratorFilter filters[] = {MecsIteratorFilter::Access, MecsIteratorFilter::Access, MecsIteratorFilter::Access};
            MecsDefineSystemInfo systemInfo {};
            systemInfo.numComponents = 3;
            systemInfo.pComponents = components;
            systemInfo.pFilters = filters;
            systemInfo.onEntityAdded = onEntityAdded_SystemACD;
            systemInfo.systemRun = systemRun_SystemACD;
            systemInfo.onEntityRemoved = onEntityRemoved_SystemACD;
            systemInfo.systemData = &acdSystem;
            MecsSystemID abSystemID = mecsWorldDefineSystem(world, &systemInfo);
        }

        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 0);

        MecsEntityID ent = mecsWorldSpawnEntity(world, {});
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 0);

        mecsWorldAddComponent(world, ent, Component_ComponentA);
        mecsWorldAddComponent(world, ent, Component_ComponentC);
        mecsWorldFlushEvents(world, nullptr);

        // ABSystem operates on entities that have a ComponentA and a ComponentB
        // ent only has a ComponentA, so it cannot iterate on it
        REQUIRE(abSystem.numEntities == 0);

        mecsWorldAddComponent(world, ent, Component_ComponentB);
        mecsWorldFlushEvents(world, nullptr);
        // Now that we added a ComponentB to ent abSystem should be notified of it
        REQUIRE(abSystem.numEntities == 1);
        // ACD's entities should be at 0
        REQUIRE(acdSystem.numEntities == 0);

        MecsEntityID ent2 = mecsWorldDuplicateEntity(world, world, ent);
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 2);


        MecsEntityID ent3 = mecsWorldSpawnEntity(world, {});
        mecsWorldAddComponent(world, ent3, Component_ComponentA);
        mecsWorldAddComponent(world, ent3, Component_ComponentC);
        mecsWorldAddComponent(world, ent3, Component_ComponentD);
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 2);
        REQUIRE(acdSystem.numEntities == 1);

        mecsWorldAddComponent(world, ent3, Component_ComponentB);
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 3);
        REQUIRE(acdSystem.numEntities == 1);

        mecsWorldRemoveComponent(world, ent, Component_ComponentA);
        mecsWorldFlushEvents(world, nullptr);
        // Once we remove one of the components that SystemAB matches
        // the system is notified of that
        REQUIRE(abSystem.numEntities == 2);
        REQUIRE(acdSystem.numEntities == 1);

        mecsWorldRemoveComponent(world, ent, Component_ComponentB);
        mecsWorldFlushEvents(world, nullptr);
        // Once an entity is removed, the system shouldn't receive further remove events for that entity
        REQUIRE(abSystem.numEntities == 2);
        REQUIRE(acdSystem.numEntities == 1);

        mecsWorldDestroyEntity(world, ent2);
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 1);
        REQUIRE(acdSystem.numEntities == 1);

        mecsWorldDestroyEntity(world, ent3);
        mecsWorldFlushEvents(world, nullptr);
        REQUIRE(abSystem.numEntities == 0);
        REQUIRE(acdSystem.numEntities == 0);

        mecsWorldFree(world);
        mecsRegistryFree(registry);
    }

    SECTION("C++ system API")
    {
        MecsRegistryCreateInfo regInfo {};
        regInfo.memAllocator = kDebugAllocator;
        mecs::Registry registry(regInfo);

        registry.addRegistration<ComponentA>();
        registry.addRegistration<ComponentB>();
        registry.addRegistration<ComponentC>();
        registry.addRegistration<ComponentD>();

        registry.addRegistration<Transform>();
        registry.addRegistration<Mesh>();
        registry.addRegistration<DirectionalLight>();
        registry.addRegistration<SpotLight>();
        registry.addRegistration<PointLight>();

        class SystemAB_Cpp {
        public:
            int counter {0};

            void onEntityAdded(mecs::World& world, mecs::EntityID entity)
            {
                counter++;
            }

            void systemRun(mecs::World& world, mecs::Iterator<ComponentA&, ComponentB&>& iterator)
            {

            }

            void onEntityRemoved(mecs::World& world, mecs::EntityID entity)
            {
                counter--;
            }
        } abSystem {};


        class RenderingSystem {
        public:
            int numMeshes {0};
            int numDirectionalLights {0};
            int numPointLights {0};
            int numSpotLights {0};

            void onMeshAdded(mecs::World& world, mecs::EntityID entity)
            {
                numMeshes++;
            }

            void updateMeshes(mecs::World& world, mecs::Iterator<Transform&, Mesh&>& iterator)
            {

            }

            void onMeshRemoved(mecs::World& world, mecs::EntityID entity)
            {
                numMeshes--;
            }

            void onDirectionalLightAdded(mecs::World& world, mecs::EntityID entity)
            {
                numDirectionalLights++;
            }

            void updateDirectionalLights(mecs::World& world, mecs::Iterator<Transform&, DirectionalLight&>& iterator)
            {

            }

            void onDirectionalLightRemoved(mecs::World& world, mecs::EntityID entity)
            {
                numDirectionalLights--;
            }

            void onPointLightLightAdded(mecs::World& world, mecs::EntityID entity)
            {
                numPointLights++;
            }

            void updatePointLightLights(mecs::World& world, mecs::Iterator<Transform&, PointLight&>& iterator)
            {

            }

            void onPointLightRemoved(mecs::World& world, mecs::EntityID entity)
            {
                numPointLights--;
            }


            void onSpotLightAdded(mecs::World& world, mecs::EntityID entity)
            {
                numSpotLights++;
            }

            void updateSpotLightLights(mecs::World& world, mecs::Iterator<Transform&, SpotLight&>& iterator)
            {

            }

            void onSpotLightRemoved(mecs::World& world, mecs::EntityID entity)
            {
                numSpotLights--;
            }

        } renderingSystem {};

        mecs::World world(registry);

        /** Simple API: binding a system that will interact with only one component set
        *   At least the systemRun method is required:
        *    void systemRun(mecs::World& world, mecs::Iterator<Components...>& iterator);
        *   The following methods are optional
        *    void onEntityAdded(mecs::World& world, mecs::EntityID entity);
        *    void onEntityRemoved(mecs::World& world, mecs::EntityID entity);
        **/
        world.addSystem<SystemAB_Cpp>(&abSystem);

        /*  Explicit API: useful for having one system interact with multiple component sets
        *   In this case we have the same RenderingSystem that has to interact with multiple
        *   different rendering-related components (e.g to add them to a scene)
        */
        world.addSystem<RenderingSystem,
                        &RenderingSystem::updateMeshes,
                        &RenderingSystem::onMeshAdded,
                        &RenderingSystem::onMeshRemoved>(&renderingSystem);
        world.addSystem<RenderingSystem,
                        &RenderingSystem::updateDirectionalLights,
                        &RenderingSystem::onDirectionalLightAdded,
                        &RenderingSystem::onDirectionalLightRemoved>(&renderingSystem);
        world.addSystem<RenderingSystem,
                        &RenderingSystem::updatePointLightLights,
                        &RenderingSystem::onPointLightLightAdded,
                        &RenderingSystem::onPointLightRemoved>(&renderingSystem);
        world.addSystem<RenderingSystem,
                        &RenderingSystem::updateSpotLightLights,
                        &RenderingSystem::onSpotLightAdded,
                        &RenderingSystem::onSpotLightRemoved>(&renderingSystem);

        mecs::EntityID entityA = world.spawnEntity();
        world.flushEvents();

        REQUIRE(abSystem.counter == 0);

        world.entityAddComponent<ComponentA>(entityA);
        world.flushEvents();
        REQUIRE(abSystem.counter == 0);
        world.entityAddComponent<ComponentB>(entityA);
        world.flushEvents();
        REQUIRE(abSystem.counter == 1);
        world.entityAddComponent<ComponentC>(entityA);
        world.flushEvents();
        REQUIRE(abSystem.counter == 1);

        std::vector<mecs::EntityID> meshes;
        std::vector<mecs::EntityID> onlyTransforms;
        std::vector<mecs::EntityID> spotLights;
        std::vector<mecs::EntityID> pointLights;
        for (int i = 0; i < 10; i ++) {
            meshes.push_back(world.spawnEntity()
                .withComponent<Transform>()
                .withComponent<Mesh>());
        }
        for (int i = 0; i < 5; i ++) {
            onlyTransforms.push_back(world.spawnEntity()
                .withComponent<Transform>());
        }


        world.flushEvents();

        REQUIRE(renderingSystem.numMeshes == 10);

        world.spawnEntity()
            .withComponent<Transform>()
            .withComponent<DirectionalLight>();
        world.flushEvents();

        for (int i = 0; i < 3; i ++) {
            spotLights.push_back(world.spawnEntity()
                .withComponent<SpotLight>()
                .withComponent<Transform>());
        }
        world.flushEvents();

        for (int i = 0; i < 6; i ++) {
            pointLights.push_back(world.spawnEntity()
                .withComponent<PointLight>()
                .withComponent<Transform>());
            world.flushEvents();
        }
        world.flushEvents();

        REQUIRE(renderingSystem.numMeshes == 10);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 3);
        REQUIRE(renderingSystem.numPointLights == 6);
        for (int i = 0; i < 5; i ++) {
            onlyTransforms.push_back(world.spawnEntity()
                .withComponent<Transform>());
        }
        world.flushEvents();
        REQUIRE(renderingSystem.numMeshes == 10);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 3);
        REQUIRE(renderingSystem.numPointLights == 6);

        for (mecs::EntityID transform : onlyTransforms) {
            world.destroyEntity(transform);
        }
        world.flushEvents();
        REQUIRE(renderingSystem.numMeshes == 10);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 3);
        REQUIRE(renderingSystem.numPointLights == 6);

        for (mecs::EntityID mesh : meshes) {
            world.destroyEntity(mesh);
        }
        world.flushEvents();
        REQUIRE(renderingSystem.numMeshes == 0);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 3);
        REQUIRE(renderingSystem.numPointLights == 6);

        for (mecs::EntityID spotLight : spotLights) {
            world.entityRemoveComponent<SpotLight>(spotLight);
        }
        world.flushEvents();
        REQUIRE(renderingSystem.numMeshes == 0);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 0);
        REQUIRE(renderingSystem.numPointLights == 6);

        for (mecs::EntityID pointLight : pointLights) {
            world.entityAddComponent<Mesh>(pointLight);
        }
        world.flushEvents();

        REQUIRE(renderingSystem.numMeshes == 6);
        REQUIRE(renderingSystem.numDirectionalLights == 1);
        REQUIRE(renderingSystem.numSpotLights == 0);
        REQUIRE(renderingSystem.numPointLights == 6);

        world.acquireIterator<mecs::EntityID>().forEach([&world](mecs::EntityID entity) {
            world.destroyEntity(entity);
        });
        world.flushEvents();

        REQUIRE(renderingSystem.numMeshes == 0);
        REQUIRE(renderingSystem.numDirectionalLights == 0);
        REQUIRE(renderingSystem.numSpotLights == 0);
        REQUIRE(renderingSystem.numPointLights == 0);


    }
}

// TEST_CASE("C++ resource management")
// {
//     static MecsSize gNumConstructorCalls = 0;
//     static MecsSize gNumDestructorCalls = 0;
//     {
//         struct Name {
//             std::string name;
//         };

//         struct VectorNames {
//             std::vector<std::string> names;
//         };

//         struct ConstructorDestructor {
//             ConstructorDestructor()
//             {
//                 gNumConstructorCalls++;
//             }
//             ConstructorDestructor(const ConstructorDestructor&) {

//             };
//             ~ConstructorDestructor()
//             {
//                 gNumDestructorCalls++;
//             }
//         };

//         mecs::Registry registry(MecsRegistryCreateInfo { .memAllocator = kDebugAllocator });
//         registry.addRegistration<Name>("Name");
//         registry.addRegistration<VectorNames>("VectorNames");
//         registry.addRegistration<ConstructorDestructor>("ConstructorDestructor");

//         mecs::PrefabID baseName = registry.createPrefab().withComponent<Name>("Mayo");
//         mecs::PrefabID vectorOfNames = registry.createPrefab().withComponent<VectorNames>(VectorNames({ "Foo", "Bar", "Baz" }));
//         mecs::PrefabID constructorDestructor = registry.createPrefab().withComponent<ConstructorDestructor>();

//         // Two constructors should have been called
//         // One for the default-initialized value
//         // One for the explicit default value that was passed to withComponent
//         // The destructor should have been called once for the explicit default value
//         REQUIRE(gNumConstructorCalls == 2);
//         REQUIRE(gNumDestructorCalls == 1);

//         mecs::World world(registry);
//         mecs::EntityID entity = world.spawnEntityPrefab(baseName);

//         mecs::EntityID entity2 = world.spawnEntityPrefab(constructorDestructor);
//         REQUIRE(gNumConstructorCalls == 3);
//         REQUIRE(gNumDestructorCalls == 1);
//         world.entityAddComponent<Name>(entity2);

//         // Constructor for the new row should have been called (+1 c)
//         // Copy constructor should have been called from old row to new row
//         // Destructor should have been called on old row
//         REQUIRE(gNumConstructorCalls == 4);
//         REQUIRE(gNumDestructorCalls == 2);
//         world.destroyEntity(entity2);
//         world.flushEvents();

//         // Destructor should have been called on row
//         REQUIRE(gNumConstructorCalls == 4);
//         REQUIRE(gNumDestructorCalls == 3);

//         {
//             Name& n = world.entityGetComponent<Name>(entity);
//             REQUIRE(n.name == "Mayo");
//             n.name = "Tizio";
//         }
//         world.entityAddComponent<VectorNames>(entity);
//         {
//             // Need another pointer because the entity was moved to another storage
//             Name& n = world.entityGetComponent<Name>(entity);
//             REQUIRE(n.name == "Tizio");

//             Name* pb = registry.getPrefabComponent<Name>(baseName);
//             REQUIRE(pb->name == "Mayo");

//             VectorNames& vn = world.entityGetComponent<VectorNames>(entity);
//             REQUIRE(vn.names.empty());
//         }
//     }
//     // No further constructors should have been called
//     REQUIRE(gNumConstructorCalls == 4);
//     // Prefab destructor should have been called now
//     REQUIRE(gNumDestructorCalls == 4);
// }

// TEST_CASE("Order of operations")
// {
//     mecs::Registry reg({ kDebugAllocator });

//     struct Foo { };
//     struct Bar { };
//     reg.addRegistration<Foo>("Foo");
//     reg.addRegistration<Bar>("Bar");

//     mecs::World world(reg);
//     mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
//     mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

//     mecs::EntityID entity = world.spawnEntity()
//                                 .withComponent<Foo>();

//     // Entities aren't spawned until flushEvents
//     REQUIRE(mecs::utils::count(fooIterator) == 0);
//     world.flushEvents();

//     REQUIRE(mecs::utils::count(fooIterator) == 1);

//     // A new archetype <Foo, Bar> is created
//     world.entityAddComponent<Bar>(entity);
//     // Archetype changes are instantaneous
//     REQUIRE(mecs::utils::count(fooIterator) == 0);
//     // But aqcuired iterators are only updated during flushEvents()
//     REQUIRE(mecs::utils::count(barIterator) == 0);
//     world.flushEvents();
//     REQUIRE(mecs::utils::count(barIterator) == 1);

//     world.destroyEntity(entity);
//     // Entities aren't destroyed until flushEvents
//     REQUIRE(mecs::utils::count(barIterator) == 1);
//     world.flushEvents();
//     // Entities aren't destroyed until flushEvents
//     REQUIRE(mecs::utils::count(barIterator) == 0);
// }

// TEST_CASE("Iteration filters")
// {
//     mecs::Registry reg({ kDebugAllocator });

//     struct Foo {
//         int x;
//     };
//     struct Bar {
//         int y { 2 };
//     };
//     struct Baz { };
//     struct ID {
//         int id;
//     };
//     reg.addRegistration<Foo>("Foo");
//     reg.addRegistration<Bar>("Bar");
//     reg.addRegistration<Baz>("Baz");
//     reg.addRegistration<ID>("ID");

//     mecs::World world(reg);
//     mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
//     mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

//     mecs::EntityID ent0 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<ID>(0);
//     mecs::EntityID ent1 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<Bar>()
//                               .withComponent<ID>(1);
//     mecs::EntityID ent2 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<Bar>()
//                               .withComponent<Baz>()
//                               .withComponent<ID>(2);
//     mecs::EntityID ent3 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<Baz>()
//                               .withComponent<ID>(3);

//     {
//         // Iterate all components with a Foo
//         mecs::Iterator allFoos = world.acquireIterator<const Foo&>();
//         REQUIRE(mecs::utils::count(allFoos) == 4);
//     }

//     {
//         // Iterate all components with a Foo that do not have a Bar
//         mecs::Iterator allFoosNoBar = world.acquireIterator<const Foo&, mecs::Not<Bar>>();
//         REQUIRE(mecs::utils::count(allFoosNoBar) == 2);
//     }

//     {
//         // Iterate all components with a Foo, a Bar that do not have a Baz
//         mecs::Iterator allFooBazNoBar = world.acquireIterator<Foo&, const Bar&, mecs::Not<Baz>>();
//         REQUIRE(mecs::utils::count(allFooBazNoBar) == 1);
//         allFooBazNoBar.forEach([&](Foo& foo, const Bar& bar, mecs::Not<Baz>) {
//             foo.x += bar.y;
//         });

//         REQUIRE(world.entityGetComponent<Foo>(ent0).x == 0);
//         REQUIRE(world.entityGetComponent<Foo>(ent1).x == 2);
//         REQUIRE(world.entityGetComponent<Foo>(ent2).x == 0);
//         REQUIRE(world.entityGetComponent<Foo>(ent3).x == 0);
//     }
// }
// TEST_CASE("Iteration filters 2")
// {
//     mecs::Registry reg({ kDebugAllocator });

//     struct Foo {
//         int x;
//     };
//     struct Bar {
//         int y { 2 };
//     };
//     struct Baz { };
//     struct ID {
//         int id;
//     };
//     reg.addRegistration<Foo>("Foo");
//     reg.addRegistration<Bar>("Bar");
//     reg.addRegistration<Baz>("Baz");
//     reg.addRegistration<ID>("ID");

//     mecs::World world(reg);
//     mecs::Iterator fooIterator = world.acquireIterator<const Foo&>();
//     mecs::Iterator barIterator = world.acquireIterator<const Bar&>();

//     mecs::EntityID ent0 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<Baz>()
//                               .withComponent<ID>(0);
//     mecs::EntityID ent1 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<Bar>()
//                               .withComponent<ID>(1);
//     mecs::EntityID ent2 = world.spawnEntity()
//                               .withComponent<Foo>()
//                               .withComponent<ID>(0);

//     {
//         // Iterate all components with a Foo and a Bar
//         mecs::Iterator allFoos = world.acquireIterator<const Foo&, const Baz&>();
//         REQUIRE(mecs::utils::count(allFoos) == 1);
//     }
//     {
//         // Iterate all components with a Foo, not a Baz
//         mecs::Iterator allFoos = world.acquireIterator<const Foo&, mecs::Not<Baz>>();
//         REQUIRE(mecs::utils::count(allFoos) == 2);
//     }
// }

// // NOLINTEND this is a test file
