#include "private.h"
#include "mecs/base.h"
#include <cstring>

const ElementInfo kVersionInfo = { .size = sizeof(Version),
    .align = _Alignof(Version) };
const ElementInfo kSizeInfo = { .size = sizeof(MecsSize),
    .align = _Alignof(MecsSize) };

void* mecsDefaultMalloc(void* userData, MecsSize size, MecsSize align) { return malloc(size); }
void* mecsDefaultRealloc(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize)
{
    return realloc(old, newSize);
}
void mecsDefaultFree(void* userData, void* ptr) { free(ptr); }

void mecsMemCpy(const char* source, MecsSize sourceSize, char* dest, MecsSize destSize)
{
    memcpy(dest, source, destSize);
}

MecsSize mecsStrLen(const char* str)
{
    MecsSize strLen = 0;
    while (str[strLen] != 0) {
        strLen++;
    }
    return strLen;
}

bool mecsStrEqual(const char* strA, const char* strB)
{
    if ((strA != nullptr) && strA == strB) { return true; }
    MecsSize lenA = mecsStrLen(strA);
    MecsSize lenB = mecsStrLen(strB);
    if (lenA != lenB) { return false; }
    for (MecsSize i = 0; i < lenA; i++) {
        if (strA[i] != strB[i]) { return false; }
    }
    return true;
}

const char* mecsStrDup(const MecsAllocator& alloc, const char* str)
{
    if (str == nullptr) { return nullptr; }
    MecsSize strLen = mecsStrLen(str);

    char* dup = mecsCalloc<char>(alloc, strLen + 1);
    mecsMemCpy(str, strLen, dup, strLen);
    dup[strLen] = 0;
    return dup;
}

MecsComponentInstanceID ComponentStorage::constructUninitialized(const MecsAllocator& allocator, MecsEntityID owner, void** outPtr)
{

    MecsSize index;
    if (mFreeIndices.count() > 0) {
        index = mFreeIndices.pop();
        mEntityToIndices.insert(allocator, owner, index);
    } else {
        index = mComponentStorage.push(allocator, nullptr);
        mEntityToIndices.insert(allocator, owner, index);
    }
    void* storage = mComponentStorage.at(index);
    if (outPtr != nullptr) { *outPtr = storage; }
    return index;
}

bool ComponentStorage::hasComponent(MecsEntityID owner) const
{
    return mEntityToIndices.contains(owner);
}

void* ComponentStorage::getComponent(MecsEntityID entity)
{
    MECS_ASSERT(mEntityToIndices.contains(entity));
    MecsSize componentIndex = mEntityToIndices.get(entity);
    return mComponentStorage.at(componentIndex);
}

void ComponentStorage::destroyInstance(const MecsAllocator& allocator, MecsEntityID owner, MecsComponentID component)
{
    MECS_ASSERT(mEntityToIndices.contains(owner));
    MecsSize componentIndex = mEntityToIndices.get(owner);

    bool removal = mEntityToIndices.remove(owner);
    MECS_ASSERT(removal);
}

void ComponentStorage::destroy(const MecsAllocator& allocator)
{
    mComponentStorage.destroy(allocator);
    mEntityToIndices.destroy(allocator);
    mFreeIndices.destroy(allocator);
}

ComponentBucket* worldGetBucket(MecsWorld* world, MecsComponentID componentID)
{
    MECS_ASSERT(world);
    if (!world->componentBuckets.isValid(componentID)) {
        return nullptr;
    }
    return &world->componentBuckets[componentID];
}