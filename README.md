# MECS - Mayo's Entity Component System
Mecs is a small archetype-based entity component system meant to be kept as small as possible.
It offers a simple C23 api, with a C++23 wrapper for improved ease of use, RAII-based resource management and better type safety.
The only dependencies are libc for the C api and catch2 (sourced in the repository), the C++ wrapper depends on std, but don't use any of the std containers.
A simple mecs C application looks like this (taken from the `tests/samples.cc` test file):
```c
#include "mecs/base.h"

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
    ComponentInfo info = { .name = "Position", .size = sizeof(Position), .align = alignof(Position) /*, .init = nullptr, .copy = nullptr, .destroy = nullptr*/ };
    // the init, copy and destroy functions are called respectively when a component is first added (both to a prefab or an entity),
    // copied or destroyed from an entity.
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
```
while the same sample, written in C++, would look like this (notice how it's much more shorter and concise)
```c++

#include "mecshpp/mecs.hpp"
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

MecsRegistryCreateInfo regInfo {};
regInfo.memAllocator = kDebugAllocator;
mecs::Registry registry(regInfo);
registry.addRegistration<Player>("Player");
registry.addRegistration<Enemy>("Enemy");
registry.addRegistration<Position>("Position");
registry.addRegistration<Velocity>("Velocity");
registry.addRegistration<Name>("Name");
registry.addRegistration<Mesh>("Mesh");

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

    // Remember to call flush
    world.flushEvents();
}
```

Additionally, by default the C++ api automatically registers components using the component's constructor as the init function, destructor as the destroy function and copy constructor as the copy function. 

## Building the library and running tests
The library is meant to be as easily buildable as possible: just
```
cmake -B build && cmake --build build
``` should suffice.

To build the tests, you can run the following commands:

```
cmake -B build -DMECS_COMPILE_TESTS=ON
```
this will build the `mecs_tests` executable target, which you can run to execute the library's tests.

Additionally, you can pass `-DMECS_TESTS_LEAK_DETECTION=ON` to cmake, which will build the tests using a custom `MecsAllocator` memory allocator that tracks allocations and frees, to check if there are any memory leaks.

Keep in mind that the custom allocator only tracks library allocations, it doesn't check for e.g structs that allocate memory on their constructors, but don't free it in their destructors.
