
#include "catch_amalgamated.hpp"
#include "mecs/base.h"
#include "mecshpp/mecs.hpp"
#include "test_private.hpp"
#include <print>
#include <string>

/// NOLINTBEGIN

TEST_CASE("C sample")
{
    struct Position {
        int x, y, z;
    };

    struct Velocity {
        int x, y, z;
    };

    struct Name {
        const char* name;
    };
    struct Player { };
    struct Enemy { };
    struct Mesh {
        std::string meshName;
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;

    /*
    The registry holds all state shared between worlds, such as registered components
    and registered prefabs
    */
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);

    // Register a component: the MECS_REGISTER_COMPONENT ensures that a matching name, size and align are given
    MECS_REGISTER_COMPONENT(registry, Player);
    MECS_REGISTER_COMPONENT(registry, Enemy);
    MECS_REGISTER_COMPONENT(registry, Velocity);
    MECS_REGISTER_COMPONENT(registry, Name);
    MECS_REGISTER_COMPONENT(registry, Mesh);

    // MECS_REGISTER_COMPONENT effectively expands to this
    static MecsComponentID Component_Position = MECS_INVALID;
    {
        ComponentInfo info = { .name = "Position", .size = sizeof(Position), .align = alignof(Position) };
        Component_Position = mecsRegistryAddRegistration(registry, &info);
    }

    // Register some prefabs, which are just blueprints for creating an entity
    // Player moves 3 pixels to right each frame
    Velocity defaultVelocity { 3, 0, 0 };
    Name playerName { "Player" };
    MecsPrefabID playerCharacterPrefab = mecsRegistryCreatePrefab(registry);
    mecsRegistryPrefabAddComponent(registry, playerCharacterPrefab, Component_Player);
    mecsRegistryPrefabAddComponent(registry, playerCharacterPrefab, Component_Position);
    mecsRegistryPrefabAddComponentWithDefaults(registry, playerCharacterPrefab, Component_Velocity, &defaultVelocity);
    mecsRegistryPrefabAddComponentWithDefaults(registry, playerCharacterPrefab, Component_Name, &playerName);

    MecsPrefabID enemyCharacterPrefab = mecsRegistryCreatePrefab(registry);
    Name enemyName { "Player" };
    mecsRegistryPrefabAddComponent(registry, enemyCharacterPrefab, Component_Enemy);
    mecsRegistryPrefabAddComponent(registry, enemyCharacterPrefab, Component_Position);
    mecsRegistryPrefabAddComponent(registry, enemyCharacterPrefab, Component_Velocity);
    mecsRegistryPrefabAddComponentWithDefaults(registry, enemyCharacterPrefab, Component_Enemy, &enemyName);

    MecsPrefabID meshPrefab = mecsRegistryCreatePrefab(registry);
    mecsRegistryPrefabAddComponent(registry, meshPrefab, Component_Mesh);
    mecsRegistryPrefabAddComponent(registry, meshPrefab, Component_Position);

    MecsWorld* world = mecsWorldCreate(registry, nullptr);
    // Prefabs can now used to spawn an entity, without manually adding its components: the entities will clone the parent components
    MecsEntityID player = mecsWorldSpawnEntityPrefab(world, playerCharacterPrefab, nullptr);

    MecsEntityID enemy0 = mecsWorldSpawnEntityPrefab(world, enemyCharacterPrefab, nullptr);
    {
        // An entity component value can be still set directly
        Position* pos = (Position*)mecsWorldEntityGetComponent(world, enemy0, Component_Position);
        *pos = { -9, 0, 0 };
    }
    MecsEntityID enemy1 = mecsWorldSpawnEntityPrefab(world, enemyCharacterPrefab, nullptr);
    {
        Position* pos = (Position*)mecsWorldEntityGetComponent(world, enemy1, Component_Position);
        *pos = { 9, 0, 0 };
    }
    MecsEntityID enemy2 = mecsWorldSpawnEntityPrefab(world, enemyCharacterPrefab, nullptr);
    {
        Position* pos = (Position*)mecsWorldEntityGetComponent(world, enemy2, Component_Position);
        *pos = { 15, 0, 0 };
    }

    for (int i = 0; i < 100; i++) {
        mecsWorldSpawnEntityPrefab(world, meshPrefab, nullptr);
    }

    // Always call this at the end of an update cycle
    mecsWorldFlushEvents(world);

    // Acquire some iterators, which are used to iterate over tuples of components
    // The first step is to acquire an iterator
    MecsIterator* playerIterator = mecsWorldAcquireIterator(world);
    {
        // The you define over what it iterates
        mecsIterComponent(playerIterator, Component_Player, 0);
        mecsIterComponent(playerIterator, Component_Position, 1);
        mecsIterComponent(playerIterator, Component_Velocity, 2);

        // Then you MUST call this function, to setup the iterator state.
        // Once an iterator has been finalized, the only way to change what it iterates on is to release the iterator and acquire a new one
        mecsIteratorFinalize(playerIterator);
    }
    MecsIterator* enemyIterator = mecsWorldAcquireIterator(world);
    {
        mecsIterComponent(enemyIterator, Component_Enemy, 0);
        mecsIterComponent(enemyIterator, Component_Position, 1);
        mecsIterComponent(enemyIterator, Component_Velocity, 2);
        mecsIteratorFinalize(enemyIterator);
    }
    MecsIterator* allPositionVelocities = mecsWorldAcquireIterator(world);
    {
        mecsIterComponent(allPositionVelocities, Component_Position, 1);
        mecsIterComponent(allPositionVelocities, Component_Velocity, 2);
        mecsIteratorFinalize(allPositionVelocities);
    }
    MecsIterator* playerThatAreEnemies = mecsWorldAcquireIterator(world);
    {
        mecsIterComponent(playerThatAreEnemies, Component_Player, 1);
        mecsIterComponent(playerThatAreEnemies, Component_Enemy, 2);
        mecsIteratorFinalize(playerThatAreEnemies);
    }
    MecsIterator* meshes = mecsWorldAcquireIterator(world);
    {
        mecsIterComponent(meshes, Component_Mesh, 1);
        mecsIterComponent(meshes, Component_Position, 2);
        mecsIteratorFinalize(meshes);
    }

    MecsIterator* allEntities = mecsWorldAcquireIterator(world);
    {
        // Even if the iterator iterates on all entities, you still need to finalize it.
        mecsIteratorFinalize(allEntities);
    }
    // One player and three enemies were just spawned
    REQUIRE(mecsUtilIteratorCount(playerIterator) == 1);
    REQUIRE(mecsUtilIteratorCount(enemyIterator) == 3);

    // There are no entities that are both players and enemies
    REQUIRE(mecsUtilIteratorCount(playerThatAreEnemies) == 0);
    REQUIRE(mecsUtilIteratorCount(meshes) == 100);

    // 100 meshes + 3 enemies + 1 player
    REQUIRE(mecsUtilIteratorCount(allEntities) == 104);

    // Run 10 iterations
    for (int i = 0; i < 10; i++) {

        // To start interating, call mecsIteratorBegin on the iterator
        // This can be called at any time during the iterator's life cycle, as long as it was correctly finalized
        mecsIteratorBegin(playerIterator);
        // When this function returns false, the iterator is at the end
        while (mecsIteratorAdvance(playerIterator)) {
            Position* playerPos = (Position*)mecsIteratorGetArgument(playerIterator, 1);
            Velocity* playerVel = (Velocity*)mecsIteratorGetArgument(playerIterator, 2);
            playerPos->x += playerVel->x;
            playerPos->y += playerVel->y;
            playerPos->z += playerVel->z;

            mecsIteratorBegin(enemyIterator);
            while (mecsIteratorAdvance(enemyIterator)) {
                Position* enemyPos = (Position*)mecsIteratorGetArgument(enemyIterator, 1);
                Velocity* enemyVel = (Velocity*)mecsIteratorGetArgument(enemyIterator, 2);
                if (enemyPos->x == playerPos->x && enemyPos->y == playerPos->y && enemyPos->z == playerPos->z) {
                    printf("Kill enemy entity %u\n", mecsIteratorGetEntity(enemyIterator));
                    mecsWorldDestroyEntity(world, mecsIteratorGetEntity(enemyIterator));
                }
            }
        }

        // Remember to flush at the end of the update cycle
        mecsWorldFlushEvents(world);
    }
    // Two enemies should have been destroyed
    REQUIRE(mecsUtilIteratorCount(enemyIterator) == 1);

    // The player should still be alive
    REQUIRE(mecsUtilIteratorCount(playerIterator) == 1);
    // 100 meshes + 1 enemies + 1 player
    REQUIRE(mecsUtilIteratorCount(allEntities) == 102);

    // Remember to release the iterators when no longer needed.
    mecsWorldReleaseIterator(world, playerIterator);
    mecsWorldReleaseIterator(world, enemyIterator);
    mecsWorldReleaseIterator(world, allPositionVelocities);
    mecsWorldReleaseIterator(world, playerThatAreEnemies);
    mecsWorldReleaseIterator(world, meshes);

    // Not releasing an iterator when freeing the world is not an error: any unreleased iterators will be freed
    // when the world is freed.
    // mecsWorldReleaseIterator(world, allEntities);
    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

struct Position {
    int x, y, z;
};

struct Velocity {
    int x, y, z;
};

struct Name {
    std::string name;
};
struct Player { };
struct Enemy { };
struct Mesh {
    std::string meshName;
};
MECS_RTTI_SIMPLE(Position);
MECS_RTTI_SIMPLE(Velocity);
MECS_RTTI_SIMPLE(Name);
MECS_RTTI_SIMPLE(Player);
MECS_RTTI_SIMPLE(Enemy);
MECS_RTTI_SIMPLE(Mesh);

TEST_CASE("C++ sample")
{

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    mecs::Registry registry(regInfo);
    registry.addRegistration<Player>();
    registry.addRegistration<Enemy>();
    registry.addRegistration<Position>();
    registry.addRegistration<Velocity>();
    registry.addRegistration<Name>();
    registry.addRegistration<Mesh>();

    // Player moves 3 pixels to right each frame
    mecs::PrefabID playerCharacterPrefab = registry.createPrefab()
                                               .withComponent<Player>()
                                               .withComponent<Position>()
                                               .withComponent<Velocity>(3, 0, 0)
                                               .withComponent<Name>("Player");

    mecs::PrefabID enemyCharacterPrefab = registry.createPrefab()
                                              .withComponent<Enemy>()
                                              .withComponent<Position>()
                                              .withComponent<Velocity>()
                                              .withComponent<Name>("Enemy");

    mecs::PrefabID meshPrefab = registry.createPrefab()
                                    .withComponent<Mesh>()
                                    .withComponent<Position>();

    mecs::World world(registry);
    mecs::EntityID player = world.spawnEntityPrefab(playerCharacterPrefab);
    mecs::EntityID enemy0 = world.spawnEntityPrefab(enemyCharacterPrefab)
                                .setComponent<Position>(-9, 0, 0);
    mecs::EntityID enemy1 = world.spawnEntityPrefab(enemyCharacterPrefab)
                                .setComponent<Position>(9, 0, 0);
    mecs::EntityID enemy2 = world.spawnEntityPrefab(enemyCharacterPrefab)
                                .setComponent<Position>(15, 0, 0);

    REQUIRE(world.entityGetNumComponents(player) == 4);
    REQUIRE(world.entityGetNumComponents(enemy0) == 4);
    REQUIRE(world.entityGetNumComponents(enemy1) == 4);

    for (int i = 0; i < 100; i++) {
        world.spawnEntityPrefab(meshPrefab)
            .setComponent<Mesh>("Mesh" + std::to_string(i));
    }

    world.flushEvents();

    mecs::Iterator playerIterator = world.acquireIterator<const Player&, Position&, const Velocity&>();
    mecs::Iterator enemyIterator = world.acquireIterator<const Enemy&, Position&, const Velocity&>();

    mecs::Iterator allPositionVelocities = world.acquireIterator<Position&, const Velocity&>();
    mecs::Iterator playerThatAreEnemies = world.acquireIterator<const Enemy&, const Player&, const Position&, const Velocity&>();
    mecs::Iterator meshes = world.acquireIterator<const Mesh&, const Position&>();
    mecs::Iterator allEntities = world.acquireIterator();

    // One player and three enemies were just spawned
    REQUIRE(mecs::utils::count(playerIterator) == 1);
    REQUIRE(mecs::utils::count(enemyIterator) == 3);

    // There are no entities that are both players and enemies
    REQUIRE(mecs::utils::count(playerThatAreEnemies) == 0);
    REQUIRE(mecs::utils::count(meshes) == 100);

    // 100 meshes + 3 enemies + 1 player
    REQUIRE(mecs::utils::count(allEntities) == 104);

    // Run 10 iterations
    for (int i = 0; i < 10; i++) {

        playerIterator.begin();
        while (playerIterator.advance()) {
            auto [player, playerPos, playerVel] = playerIterator.get();
            playerPos.x += playerVel.x;
            playerPos.y += playerVel.y;
            playerPos.z += playerVel.z;

            enemyIterator.begin();
            while (enemyIterator.advance()) {
                auto [enemy, enemyPos, enemyVel] = enemyIterator.get();
                if (enemyPos.x == playerPos.x && enemyPos.y == playerPos.y && enemyPos.z == playerPos.z) {
                    std::println("Kill enemy entity {}", enemyIterator.getEntityID());
                    world.destroyEntity(enemyIterator.getEntityID());
                }
            }
        }

        world.flushEvents();
        // Remember to call flush
    }
    // Two enemies should have been destroyed
    REQUIRE(mecs::utils::count(enemyIterator) == 1);

    // The player should still be alive
    REQUIRE(mecs::utils::count(playerIterator) == 1);
    // 100 meshes + 1 enemies + 1 player
    REQUIRE(mecs::utils::count(allEntities) == 102);
}
/// NOLINTEND