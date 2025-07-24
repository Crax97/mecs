#pragma once

#include "mecs/base.h"
#include "mecs/mecs.h"

#include "collections.h"

void* mecsDefaultMalloc(void* userData, MecsSize size, MecsSize align);
void mecsDefaultFree(void* userData, void* ptr);
void* mecsDefaultRealloc(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize);

constexpr MecsAllocator kDefaultAllocator {
    .memAlloc = &mecsDefaultMalloc,
    .memRealloc = &mecsDefaultRealloc,
    .memFree = &mecsDefaultFree,
    .userData = nullptr,
};
constexpr MecsAllocator kNullAllocator {
    .memAlloc = nullptr,
    .memRealloc = nullptr,
    .memFree = nullptr,
    .userData = nullptr,
};

using EntityTag = struct EntityTagT {
    MecsU32 index      : 24;
    MecsU32 generation : 8;
};

using TaggedEntity = union TaggedEntityT {
    MecsEntityID entityID;
    EntityTag tag;
};

static_assert(sizeof(TaggedEntity) == sizeof(MecsEntityID));

class ComponentStorage {
public:
    ComponentStorage() = default;
    ComponentStorage(MecsAllocator allocator, ElementInfo info)
        : mComponentStorage(info)
    {
    }

    MecsComponentInstanceID constructUninitialized(const MecsAllocator& allocator, MecsEntityID owner, void** outPtr);
    [[nodiscard]]
    bool hasComponent(MecsEntityID owner) const;
    void* getComponent(MecsEntityID entity);
    void destroyInstance(const MecsAllocator& allocator, MecsEntityID owner, MecsComponentID component);

    void destroy(const MecsAllocator& allocator);

private:
    MecsVecUnmanaged mComponentStorage;
    SparseSet<MecsEntityID> mEntityToIndices;
    MecsVec<MecsSize> mFreeIndices;
};

enum class EntityStatus : MecsU8 {
    eNewlySpawned,
    eSpawned,
    eDestroying,
};

struct MecsRegistry_t {
    MecsAllocator memAllocator;

    MecsVec<ComponentInfo> components;
};

struct MecsEntity {
    const char* name = nullptr;
    EntityStatus status = EntityStatus::eNewlySpawned;
    MecsVec<MecsComponentID> components;
};

enum class IteratorStatus : MecsU8 {
    eReleased,
    eInitializing,
    eIterating,
};
enum class ArgumentAccess : MecsU8 {
    eInOut,
    eIn,
};

enum class ArgumentType : MecsU8 {
    eComponent,
    eResource,
};

struct MecsIteratorArgument {
    ArgumentType argumentType;
    MecsComponentID argumentID;
    ArgumentAccess argumentAccess = ArgumentAccess::eInOut;
};

struct MecsWorldIterator_t {
    bool dirty = true;
    MecsWorld* world { nullptr };
    MecsVec<MecsIteratorArgument> arguments;
    MecsVec<MecsEntityID> entities;
    MecsSize currentEntity { 0 };
    MecsEntityID currentEntityID = MECS_INVALID;
    IteratorStatus status = IteratorStatus::eReleased;
};

struct ComponentBucket {
    ComponentStorage storage;
    MecsVec<MecsIterator*> usedByIterators;
};

enum class WorldEventKind : MecsU8 {
    eNewEntity, // entityID
    eDestroyEntity, // entityID
    eNewComponent, // entityID componentID
    eUpdateComponent, // entityID componentID
    eDestroyComponent, // entityID componentID
};

struct WorldEvent {
    WorldEventKind kind;
    MecsU32 entityID;
    MecsU32 componentID;
};

struct MecsWorld_t {
    MecsRegistry* registry;

    MecsAllocator memAllocator;
    GenArena<MecsEntity> entities;
    MecsVec<ComponentBucket> componentBuckets;
    MecsVec<WorldEvent> newEvents;
    MecsVec<MecsWorldIterator_t*> reusableIterators;
    MecsVec<MecsWorldIterator_t*> acquiredIterators;
};

const char* mecsStrDup(const MecsAllocator& alloc, const char* str);
bool mecsStrEqual(const char* strA, const char* strB);
MecsSize mecsStrLen(const char* str);
void mecsMemCpy(const char* source, MecsSize sourceSize, char* dest, MecsSize destSize);
ComponentBucket* worldGetBucket(MecsWorld* world, MecsComponentID componentID);