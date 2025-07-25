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

using ArchetypeID = MecsU32;

class ComponentStorage {
public:
    ComponentStorage() = default;
    ComponentStorage(MecsAllocator allocator, ElementInfo info)
        : mComponentStorage(info)
    {
    }

    MecsComponentID constructUninitialized(const MecsAllocator& allocator, MecsEntityID owner, void** outPtr);
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
    BitSet componentSet;
    MecsVec<MecsComponentID> components;
    MecsVec<ArchetypeID> archetypes;
    MecsSize currentArchetype { 0 };
    MecsSize currentRow { 0 };
    MecsEntityID currentEntityID = MECS_INVALID;
    IteratorStatus status = IteratorStatus::eReleased;
};

struct ComponentBucket {
    ComponentStorage storage;
    MecsVec<MecsIterator*> usedByIterators;
};

struct EntityRow {
    MecsSize row { MECS_INVALID };
};

/*
Stores components in columns, where each component (column) can be accessed by a row index
Given components A, B, C

   | A | B | C |
   |---|---|---|
 0 | a0| b0| c0|
 1 | a1| b1| c1|
 2 | a2| b2| c2|
 3 | a3| b3| c3|

The rows are tightly packed, and when a row is removed it is swapped with the last one
to keep the storage tightly packed
*/
class RowStorage {
public:
    RowStorage() = default;
    RowStorage(BitSet componentSet, MecsWorld* world);
    [[nodiscard]]
    bool hasComponent(MecsComponentID component) const;

    [[nodiscard]]
    void* getRowComponent(MecsComponentID component, MecsSize row) const;

    [[nodiscard]]
    MecsSize allocateRow(const MecsAllocator& alloc);

    MecsSize freeRow(const MecsAllocator& alloc, MecsSize row);

    void copyRow(MecsSize sourceRow, RowStorage& dest, MecsSize destRow);

    [[nodiscard]]
    MecsSize rows() const;

    [[nodiscard]]
    const BitSet& bitset() const
    {
        return mCmponentSet;
    }

    template <typename F>
    void forEachCompoonentInRow(MecsSize row, F&& func)
    {
        MECS_ASSERT(mTakenRows.test(row));
        mCmponentSet.forEach([&](MecsComponentID component) {
            MecsVecUnmanaged& storage = mStorages[component];
            void* ptr = storage.at(row);
            func(component, ptr);
        });
    }

    void destroy(const MecsAllocator& alloc);

private:
    MecsVecUnmanaged& getStorage(MecsComponentID component);
    MecsRegistry* mRegistry;
    BitSet mCmponentSet;
    BitSet mTakenRows;
    MecsVec<MecsVecUnmanaged> mStorages;
    MecsVec<MecsSize> mFreeIndices;
    MecsSize mCount = 0;
    MecsSize mCapacity = 0;
};

// Untyped blob of data which holds a single component instance. It ensures that it is freed in the destructor
class ComponentBlob {
public:
    ComponentBlob() noexcept = default;
    ComponentBlob(const MecsRegistry* registry, MecsComponentID component) noexcept;

    ComponentBlob(ComponentBlob&& rhs) noexcept;
    ComponentBlob& operator=(ComponentBlob&& rhs) noexcept;

    [[nodiscard]]
    void* get() const;
    void destroy(const MecsRegistry* registry);
    void copyOnto(const MecsRegistry* registry, void* dest) const;

    ~ComponentBlob();

private:
    MecsU8* mData;
    MecsComponentID mComponentID { MECS_INVALID };
};

struct MecsPrefabComponent {
    MecsComponentID component;
    ComponentBlob blob;
};
struct MecsPrefab {
    MecsVec<MecsPrefabComponent> components;
    BitSet archetypeBitset;
};

struct MecsRegistry_t {
    MecsAllocator memAllocator;

    MecsVec<ComponentInfo> components;
    GenArena<MecsPrefab> prefabs;
};
struct Archetype {
    RowStorage storage;
    MecsVec<MecsEntityID> rowToEntity; // Tracks to which entity each row belongs;
};

struct MecsEntity {
    const char* name = nullptr;
    EntityStatus status = EntityStatus::eNewlySpawned;
    ArchetypeID archetype;
    MecsSize archetypeRow;
    MecsPrefabID prefabID;
};

enum class WorldEventKind : MecsU8 {
    eNewEntity, // entityID
    eDestroyEntity, // entityID
    eNewComponent, // entityID componentID
    eUpdateComponent, // entityID componentID
    eDestroyComponent, // entityID componentID
    eNewArchetype, // entityID -> archetypeID
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
    MecsVec<Archetype> archetypes;
    MecsVec<WorldEvent> newEvents;
    MecsVec<MecsWorldIterator_t*> reusableIterators;
    MecsVec<MecsWorldIterator_t*> acquiredIterators;
};

const char* mecsStrDup(const MecsAllocator& alloc, const char* str);
bool mecsStrEqual(const char* strA, const char* strB);
MecsSize mecsStrLen(const char* str);
void mecsMemCpy(const char* source, MecsSize sourceSize, char* dest, MecsSize destSize);

ArchetypeID findNewArchetype(MecsWorld* world, ArchetypeID source, MecsComponentID component, bool include);
