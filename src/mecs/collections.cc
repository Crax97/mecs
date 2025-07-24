#include "private.h"
#include <cstring>

#include "collections.h"

char* mecsRellocAligned(const MecsAllocator& alloc, char* old, MecsSize oldCount, MecsSize newCount, MecsSize align)
{
    return static_cast<char*>(alloc.memRealloc(alloc.userData, old, oldCount, align, newCount));
}

MecsSize MecsVecUnmanaged::push(const MecsAllocator& allocator, void* value)
{
    MECS_ASSERT(allocator.memAlloc != nullptr);
    MECS_ASSERT(mElementInfo.size > 0 && mElementInfo.align > 0);
     if ((mCount + 1) > mCapacity) {
        grow(allocator, growCount(mCapacity));
    }

    mCount++;

    MecsSize elementIndex = mCount - 1;
    char* destPtr = at(elementIndex);
    if (value != nullptr) {
        mecsMemCpy(static_cast<char*>(value), mElementInfo.size, destPtr, mElementInfo.size);
    } else {
        memset(destPtr, 0, mElementInfo.size);
    }
    return elementIndex;
}
void MecsVecUnmanaged::pop(void* valuePtr)
{
    MECS_ASSERT(mCount > 0);
    char* sourcePtr = at(mCount - 1);
    if (valuePtr != nullptr) {
        mecsMemCpy(sourcePtr, mElementInfo.size, static_cast<char*>(valuePtr), mElementInfo.size);
    }
    mCount -= 1;
}

char* MecsVecUnmanaged::at(MecsSize index) const
{
    MECS_ASSERT(index < mCount && mElementInfo.size > 0 && mElementInfo.align > 0);
    char* sourcePtr = (mData + (index * mElementInfo.size));
    return sourcePtr;
}

void MecsVecUnmanaged::resize(const MecsAllocator& allocator, MecsSize newSize)
{
    MECS_ASSERT(allocator.memAlloc != nullptr);
    if (newSize == 0) {
        destroy(allocator);
    }

    char* newData = mecsRellocAligned(allocator, mData, mCapacity * mElementInfo.size, newSize * mElementInfo.size, mElementInfo.align);
    mData = newData;

    mCapacity = newSize;
    mCount = newSize;
}

void MecsVecUnmanaged::destroy(const MecsAllocator& allocator)
{
    mecsFree<char>(allocator, mData);
    mData = nullptr;
    mCount = 0;
    mCapacity = 0;
}

void MecsVecUnmanaged::grow(const MecsAllocator& allocator, MecsSize size)
{
    MECS_ASSERT(allocator.memAlloc != nullptr);
    MecsSize newCount = 1;

    if (mCapacity > 0) {
        newCount = mCapacity + size;
    }
    char* newData = mecsRellocAligned(allocator, mData, mCapacity * mElementInfo.size, newCount * mElementInfo.size, mElementInfo.align);

    mCapacity = newCount;
    mData = newData;
}