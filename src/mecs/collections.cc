#include "mecs/base.h"
#include "private.h"
#include <cstring>

#include "collections.h"

char* mecsRellocAligned(const MecsAllocator& alloc, char* old, MecsSize oldCount, MecsSize newCount, MecsSize align)
{
    return static_cast<char*>(alloc.memRealloc(alloc.userData, old, oldCount, align, newCount));
}

MecsVecUnmanaged::MecsVecUnmanaged(MecsVecUnmanaged&& rhs) noexcept
    : mElementInfo(rhs.mElementInfo)
    , mCount(rhs.mCount)
    , mCapacity(rhs.mCapacity)
    , mData(rhs.mData)
{
    rhs.mElementInfo = {};
    rhs.mCapacity = {};
    rhs.mCount = {};
    rhs.mData = nullptr;
}
MecsVecUnmanaged& MecsVecUnmanaged::operator=(MecsVecUnmanaged&& rhs) noexcept
{
    mElementInfo = rhs.mElementInfo;
    mCount = rhs.mCount;
    mCapacity = rhs.mCapacity;
    mData = rhs.mData;

    rhs.mElementInfo = {};
    rhs.mCapacity = {};
    rhs.mCount = {};
    rhs.mData = nullptr;
    return *this;
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

void BitSet::set(const MecsAllocator& allocator, MecsSize slot, bool value)
{
    const MecsSize wordSlot = slot / (sizeof(Word) * 8);
    const MecsSize wordBit = slot % (sizeof(Word) * 8);
    mWords.ensureSize(allocator, wordSlot + 1);

    Word& word = mWords[wordSlot];
    Word bitFlag = (1 << wordBit);

    if (value) {
        word |= bitFlag;
    } else {
        word = word & ~bitFlag;
    }
}

bool BitSet::test(MecsSize slot) const
{
    const MecsSize wordSlot = slot / (sizeof(Word) * 8);
    if (!mWords.isValid(wordSlot)) {
        return false;
    }
    const MecsSize wordBit = slot % (sizeof(Word) * 8);
    const MecsSize wordmask = (1 << wordBit);
    const Word word = mWords[wordSlot];
    return (word & wordmask) != 0;
}
void BitSet::clear()
{
    mWords.clear();
}

void BitSet::destroy(const MecsAllocator& allocator)
{
    mWords.destroy(allocator);
}

bool BitSet::allZeroes() const
{
    for (MecsSize i = 0; i < mWords.count(); i++) {
        if (mWords[i] != 0) {
            return false;
        }
    }

    return true;
}

bool BitSet::contains(const BitSet& other) const
{
    if (other.mWords.count() != mWords.count()) {
        return allZeroes() && other.allZeroes();
    }

    MecsSize wordCount = mWords.count();
    for (MecsSize i = 0; i < wordCount; i++) {
        if ((mWords[i] & other.mWords[i]) != mWords[i]) {
            return false;
        }
    }
    return true;
}

BitSet BitSet::clone(const MecsAllocator& allocator) const
{
    BitSet cloneSet;
    cloneSet.mWords.resize(allocator, mWords.count());
    MecsSize wordCount = mWords.count();
    for (MecsSize i = 0; i < wordCount; i++) {
        cloneSet.mWords[i] = mWords[i];
    }

    return cloneSet;
}

MecsSize BitSet::count() const
{
    MecsSize res = 0;
    const MecsSize numBits = mWords.count() * sizeof(Word);
    for (MecsSize i = 0; i < numBits; i++) {
        const Word mask = 1 << (i % sizeof(Word));
        const Word index = i / sizeof(Word);
        res += (mWords[index] & mask) > 0 ? 1 : 0;
    }
    return res;
}

bool BitSet::operator==(const BitSet& other) const
{
    if (other.mWords.count() != mWords.count()) {
        return allZeroes() == other.allZeroes();
    }

    return mWords == other.mWords;
}