#include "private.h"
#include "collections.h"
#include "mecs/base.h"
#include <cstring>

void* mecsDefaultMalloc(void* userData, MecsSize size, MecsSize align) { return malloc(size); }
void* mecsDefaultRealloc(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize)
{
    return realloc(old, newSize);
}
void mecsDefaultFree(void* userData, void* ptr)
{
    free(ptr);
}

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

    char* dup = mecsCallocAligned<char>(alloc, strLen + 1, alignof(char));
    mecsMemCpy(str, strLen, dup, strLen);
    dup[strLen] = 0;
    return dup;
}

MecsComponentID ComponentStorage::constructUninitialized(const MecsAllocator& allocator, MecsEntityID owner, void** outPtr)
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

ComponentBlob::ComponentBlob(const MecsRegistry* registry, MecsComponentID component) noexcept
    : mComponentID(component)
{
    MECS_ASSERT(registry != nullptr && registry->memAllocator.memAlloc != nullptr);
    const ComponentInfo& componentInfo = registry->components[component];
    mData = mecsCallocAligned<MecsU8>(registry->memAllocator, componentInfo.size, componentInfo.align);
    if (componentInfo.init != nullptr) {
        componentInfo.init(mData);
    }
}

ComponentBlob::ComponentBlob(ComponentBlob&& rhs) noexcept
    : mData(rhs.mData)
    , mComponentID(rhs.mComponentID)
{
    rhs.mData = nullptr;
    rhs.mComponentID = MECS_INVALID;
}
ComponentBlob& ComponentBlob::operator=(ComponentBlob&& rhs) noexcept
{
    mData = rhs.mData;
    mComponentID = rhs.mComponentID;
    rhs.mData = nullptr;
    rhs.mComponentID = MECS_INVALID;
    return *this;
}

void* ComponentBlob::get() const
{
    return static_cast<void*>(mData);
}

void ComponentBlob::destroy(const MecsRegistry* registry)
{
    if (mData == nullptr) {
        return;
    }
    const ComponentInfo& componentInfo = registry->components[mComponentID];
    if (componentInfo.destroy != nullptr) {
        componentInfo.destroy(mData);
    }
    mecsFree(registry->memAllocator, mData);
    mData = nullptr;
}

void ComponentBlob::copyOnto(const MecsRegistry* registry, void* dest) const
{
    MECS_ASSERT(registry != nullptr && registry->memAllocator.memAlloc != nullptr);
    MECS_ASSERT(mComponentID != MECS_INVALID);
    const ComponentInfo& componentInfo = registry->components[mComponentID];
    if (componentInfo.copy != nullptr) {
        componentInfo.copy(mData, dest, componentInfo.size);
    } else {
        memcpy(dest, mData, componentInfo.size);
    }
}

ComponentBlob::~ComponentBlob()
{
    MECS_ASSERT(mData == nullptr && "Did not call destroy");
}