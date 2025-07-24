#include "collections.h"
#include "mecs/base.h"
#include "private.h"
#include <cstring>

RowStorage::RowStorage(BitSet componentSet, MecsWorld* world)
    : mCmponentSet(std::move(componentSet))
    , mRegistry(world->registry)
{
    MECS_ASSERT(world->registry);
    mCmponentSet.forEach([&](MecsComponentID componentID) {
        mStorages.ensureSize(world->memAllocator, componentID + 1);
        const ComponentInfo& info = mRegistry->components[componentID];
        mStorages[componentID] = MecsVecUnmanaged(ElementInfo { info.size, info.align });
    });
}

bool RowStorage::hasComponent(MecsComponentID component) const
{
    return mCmponentSet.test(component);
}

void* RowStorage::getRowComponent(MecsComponentID component, MecsSize row) const
{
    MECS_ASSERT(hasComponent(component));
    MECS_ASSERT(mStorages.isValid(component));

    MecsVecUnmanaged& storage = mStorages[component];
    MECS_ASSERT(mTakenRows.test(row));
    return storage.at(row);
}

MecsSize RowStorage::allocateRow(const MecsAllocator& alloc)
{
    MecsSize rowIndex = 0;
    if (mFreeIndices.count() > 0) {
        rowIndex = mFreeIndices.pop();
    } else {
        rowIndex = mCapacity++;
        mCmponentSet.forEach([&](MecsComponentID component) {
            MecsVecUnmanaged& storage = getStorage(component);
            storage.push(alloc, nullptr);
        });
    }
    MECS_ASSERT(!mTakenRows.test(rowIndex));
    mTakenRows.set(alloc, rowIndex, true);
    mCmponentSet.forEach([&](MecsComponentID component) {
        ComponentInfo& info = mRegistry->components[component];
        if (!info.init) { return; }
        MecsVecUnmanaged& storage = getStorage(component);
        void* componentPtr = storage.at(rowIndex);
        info.init(componentPtr);
    });

    mCount++;
    return rowIndex;
}

MecsSize RowStorage::freeRow(const MecsAllocator& alloc, MecsSize row)
{
    MECS_ASSERT(mTakenRows.test(row));

    // Copy the last row to the current row
    // There's no need to do that if there's only one row left
    // or we're removing the last row itself
    if (mCount > 1 && row < mCount - 1) {
        mCmponentSet.forEach([&](MecsComponentID component) {
            ComponentInfo& reg = mRegistry->components[component];
            MecsVecUnmanaged& storage = getStorage(component);
            void* current = storage.at(row);
            void* last = storage.at(storage.count() - 1);
            if (reg.copy) {
                reg.copy(last, current, reg.size);
            } else {
                memcpy(current, last, reg.size);
            }
        });
    }

    // Deinitialize the last row (either it was the only one, or it got copied to the current row)
    mCmponentSet.forEach([&](MecsComponentID component) {
        ComponentInfo& reg = mRegistry->components[component];
        if (!reg.destroy) {
            return;
        }
        MecsVecUnmanaged& storage = getStorage(component);
        void* last = storage.at(mCount - 1);
        reg.destroy(last);
    });
    
    // Mark the old row as reusable
    mTakenRows.set(alloc, mCount - 1, false);
    mFreeIndices.push(alloc, mCount - 1);
    mCount--;
    return mCount;
}

void RowStorage::copyRow(MecsSize sourceRow, RowStorage& dest, MecsSize destRow)
{
    mCmponentSet.forEach([&](MecsComponentID component) {
        if (!dest.hasComponent(component)) {
            return;
        }

        void* destPtr = dest.getRowComponent(component, destRow);
        void* source = getRowComponent(component, sourceRow);
        ComponentInfo& reg = mRegistry->components[component];
        if (reg.copy != nullptr) {
            reg.copy(source, destPtr, reg.size);
        } else {
            memcpy(destPtr, source, reg.size);
        }
    });
}

void RowStorage::destroy(const MecsAllocator& alloc)
{
    while (mCount > 0) {
        freeRow(alloc, mCount - 1);
    }
    mCmponentSet.destroy(alloc);
    mTakenRows.destroy(alloc);
    mStorages.forEach([&](MecsVecUnmanaged& vec) {
        vec.destroy(alloc);
    });
    mStorages.destroy(alloc);
    mFreeIndices.destroy(alloc);
}

MecsSize RowStorage::rows() const
{
    return mCount;
}

MecsVecUnmanaged& RowStorage::getStorage(MecsComponentID component)
{
    return mStorages[component];
}