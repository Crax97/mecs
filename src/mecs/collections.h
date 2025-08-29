#pragma once

#include "mecs/base.h"

#include <concepts>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include <algorithm>
#include <utility>

template <typename T, typename... Args>
T* mecsAlloc(const MecsAllocator& alloc, Args&&... args)
{
    T* ptr = static_cast<T*>(alloc.memAlloc(alloc.userData, sizeof(T), alignof(T)));
    new (ptr) T(std::forward<Args>(args)...);
    return ptr;
}

template <typename T>
T* mecsCallocAligned(const MecsAllocator& alloc, MecsSize count, MecsSize align)
{
    T* ptr = static_cast<T*>(alloc.memAlloc(alloc.userData, sizeof(T) * count, align));
    return ptr;
}

template <typename T>
T* mecsCalloc(const MecsAllocator& alloc, MecsSize count)
{
    return mecsCallocAligned<T>(alloc, count, alignof(T));
}
template <typename T>
T* mecsRelloc(const MecsAllocator& alloc, T* old, MecsSize oldCount, MecsSize newCount)
{
    return static_cast<T*>(alloc.memRealloc(alloc.userData, old, sizeof(T) * oldCount, alignof(T), sizeof(T) * newCount));
}

char* mecsRellocAligned(const MecsAllocator& alloc, char* old, MecsSize oldCount, MecsSize newCount, MecsSize align);

template <typename T>
void mecsFree(const MecsAllocator& alloc, T* ptr)
{
    ptr->~T();
    alloc.memFree(alloc.userData, static_cast<void*>(const_cast<std::remove_const_t<T>*>(ptr)));
}

constexpr MecsSize growCount(MecsSize size)
{
    if (size == 1) {
        return 4;
    }
    // 1.5 growth factor
    return size + std::max(size / 2, 1ULL); // NOLINT not a magic number
}

using GenIndex = MecsU32;

using Version = struct VersionT {
    MecsU32 index      : 24;
    MecsU32 generation : 8;
};

using TaggedGenIndex = union TaggedGenIndexT {
    Version version;
    GenIndex index;
};

using ElementInfo = struct ElementInfoT {
    MecsSize size { 0 };
    MecsSize align { 0 };
};

template <typename T>
class MecsStdAllocator {
public:
    constexpr MecsStdAllocator(MecsAllocator allocator) noexcept
        : mAllocator(allocator)
    {
    }

    constexpr MecsStdAllocator(const MecsStdAllocator& rhs) = default;
    constexpr MecsStdAllocator& operator=(const MecsStdAllocator& rhs) = default;
    constexpr MecsStdAllocator(MecsStdAllocator&& rhs) = default;
    constexpr MecsStdAllocator& operator=(MecsStdAllocator&& rhs) = default;

    constexpr auto operator<=>(const MecsStdAllocator& rhs) const noexcept = default;
    constexpr bool operator==(const MecsStdAllocator& rhs) const noexcept
    {
        return (
            rhs.mAllocator.memAlloc == mAllocator.memAlloc && rhs.mAllocator.memRealloc == mAllocator.memRealloc && rhs.mAllocator.memFree == mAllocator.memFree && rhs.mAllocator.userData == mAllocator.userData);
    }

    template <typename U>
    constexpr MecsStdAllocator(const MecsStdAllocator<U>& other) noexcept
        : mAllocator(other.mAllocator)
    {
    }

    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    constexpr value_type* allocate(std::size_t n)
    {
        return mecsCalloc<T>(mAllocator, n);
    }
    constexpr void deallocate(T* ptr, std::size_t n)
    {
        return mecsFree(mAllocator, ptr);
    }
    constexpr std::allocation_result<T*, std::size_t> allocate_at_least(std::size_t n) /// NOLINT std function
    {
        return { mecsCalloc<T>(mAllocator, n), n };
    }

    template <typename... Args>
    void construct(T* ptr, Args&&... args)
    {
        new (ptr) T(std::forward<Args...>(args)...);
    }

    MecsAllocator mAllocator;
};

template <typename T>
class MecsVec {
public:
    MecsVec() = default;
    MecsVec(std::initializer_list<T> elems, MecsAllocator allocator)
    {
        for (const auto& elem : elems) {
            push(allocator, elem);
        }
    }

    MecsVec& operator=(MecsVec&& rhs) noexcept
    {
        mData = rhs.mData;
        mCount = rhs.mCount;
        mCapacity = rhs.mCapacity;
        rhs.mData = nullptr;
        rhs.mCapacity = 0;
        rhs.mData = 0;
        return *this;
    }

    MecsVec(MecsVec&& rhs) noexcept
        : mData(rhs.mData)
        , mCount(rhs.mCount)
        , mCapacity(rhs.mCapacity)
    {
        rhs.mData = nullptr;
        rhs.mCapacity = 0;
        rhs.mData = 0;
    }

    MecsVec(const MecsVec&) = delete;
    MecsVec& operator=(const MecsVec&) = delete;

    constexpr bool operator==(const MecsVec& other) const
        requires(std::equality_comparable<T>)
    {
        if (other.count() != count()) {
            return false;
        }

        for (MecsSize i = 0; i < mCount; i++) {
            if (other[i] != at(i)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    MecsSize count() const
    {
        return mCount;
    }

    [[nodiscard]]
    bool empty() const
    {
        return mCount == 0;
    }

    [[nodiscard]]
    bool isValid(MecsSize index) const
    {
        return index < mCount;
    }

    [[nodiscard]]
    T& back() const
    {
        MECS_ASSERT(mCount > 0);
        return at(mCount - 1);
    }

    void removeAt(MecsSize index)
    {
        MECS_ASSERT(isValid(index));
        std::swap(back(), at(index));
        pop();
    }

    bool remove(T value)
    {
        for (MecsSize removed = 0; removed < mCount; removed++) {
            if (value == at(removed)) {
                removeAt(removed);
                return true;
            }
        }
        return false;
    }

    template <typename F>
    void forEach(F&& func)
    {
        MecsSize oldCount = mCount;
        for (MecsSize i = 0; i < mCount; i++) {
            func(at(i));
            MECS_ASSERT(mCount == oldCount && "Element count changed during iteration! This is not allowed");
        }
    }

    template <typename F>
    void forEach(F&& func) const
    {
        MecsSize oldCount = mCount;
        for (MecsSize i = 0; i < mCount; i++) {
            func(at(i));
            MECS_ASSERT(mCount == oldCount && "Element count changed during iteration! This is not allowed");
        }
    }

    void clear()
    {
        mCount = 0;
    }

    MecsSize push(const MecsAllocator& allocator, T value)
    {
        MECS_ASSERT(allocator.memAlloc != nullptr);
        if (mCount + 1 > mCapacity) {
            grow(allocator, growCount(mCapacity));
        }

        mCount++;

        T* ptr = atPtr(mCount - 1);
        new (ptr) T(std::move(value));
        return mCount - 1;
    }
    MecsSize pushUnique(const MecsAllocator& allocator, T value)
    {
        for (MecsSize i = 0; i < mCount; i++) {
            if (value == at(i)) {
                return i;
            }
        }

        return push(allocator, value);
    }
    T pop()
    {
        MECS_ASSERT(mCount > 0);
        T* ptr = atPtr(mCount - 1);
        T copy = std::move(*ptr);
        ptr->~T();
        mCount--;
        return copy;
    }

    [[nodiscard]]
    T& at(MecsSize index) const
    {
        return *atPtr(index);
    }

    [[nodiscard]]
    T* atPtr(MecsSize index) const
    {
        MECS_ASSERT(index < mCount);
        return mData + index;
    }

    void resize(const MecsAllocator& allocator, MecsSize newSize)
    {
        if (newSize == mCount) {
            return;
        }
        if (newSize == 0) {
            destroy(allocator);
            return;
        }
        MecsSize oldCount = mCount;
        mCount = newSize;

        if (newSize < oldCount) {
            for (MecsSize i = newSize; i < mCount; i++) {
                T* ptr = atPtr(i);
                ptr->~T();
            }

            mData = mecsRelloc(allocator, mData, mCapacity, newSize);
            mCapacity = newSize;
        } else if (newSize > oldCount) {
            if (newSize >= mCapacity) {
                // Need to resize the Vec to accomodate for more items
                grow(allocator, growCount(newSize));
            }

            // Check if the type is not POD, only in that case call the constructor
            // Otherwise just mallocing + memset memory is enough
            if constexpr (std::is_default_constructible_v<T>) {
                // if it's a pod type just do a memset(0)
                if (std::is_standard_layout_v<T>) {
                    T* ptr = atPtr(oldCount);
                    memset(static_cast<void*>(ptr), 0, sizeof(T) * (newSize - oldCount));
                } else {
                    for (MecsSize i = oldCount; i < newSize; i++) {
                        T* ptr = atPtr(i);
                        new (ptr) T {};
                    }
                }
            }
        }
    }

    void ensureSize(const MecsAllocator& allocator, MecsSize size)
    {
        if (mCount < size) {
            resize(allocator, size);
        }
    }

    void destroy(const MecsAllocator& allocator)
    {
        if (mData == nullptr) { return; }
        mecsFree(allocator, mData);
        mCapacity = 0;
        mCount = 0;
        mData = nullptr;
    }

    T& operator[](auto index) const { return at(static_cast<MecsSize>(index)); }

    ~MecsVec()
    {
        MECS_ASSERT(mData == nullptr && "Did not call destroy");
    }

private:
    void grow(const MecsAllocator& allocator, MecsSize newCapacity)
    {
        MECS_ASSERT(newCapacity > mCapacity);
        mData = mecsRelloc(allocator, mData, mCapacity, newCapacity);
        mCapacity = newCapacity;
    }
    MecsSize mCount { 0 };
    MecsSize mCapacity { 0 };
    T* mData { nullptr };
};

class MecsVecUnmanaged {
public:
    MecsVecUnmanaged() = default;
    MecsVecUnmanaged(ElementInfo elementInfo)
        : mElementInfo(elementInfo)
    {
    }

    MecsVecUnmanaged(MecsVecUnmanaged&& rhs) noexcept;
    MecsVecUnmanaged& operator=(MecsVecUnmanaged&& rhs) noexcept;

    MecsVecUnmanaged(const MecsVecUnmanaged&) = delete;
    MecsVecUnmanaged& operator=(const MecsVecUnmanaged&) = delete;

    [[nodiscard]]
    MecsSize count() const
    {
        return mCount;
    }

    [[nodiscard]]
    bool isValid(MecsSize index) const
    {
        return index < mCount;
    }

    MecsSize push(const MecsAllocator& allocator, void* value);
    void pop(void* valuePtr);

    [[nodiscard]]
    char* at(MecsSize index) const;

    void resize(const MecsAllocator& allocator, MecsSize newSize);

    void destroy(const MecsAllocator& allocator);

    void* operator[](auto index) const { return at(static_cast<MecsSize>(index)); }

    ~MecsVecUnmanaged()
    {
        MECS_ASSERT(mData == nullptr && "Did not call destroy");
    }

private:
    void grow(const MecsAllocator& allocator, MecsSize size);
    ElementInfo mElementInfo;
    MecsSize mCount { 0 };
    MecsSize mCapacity { 0 };
    char* mData { nullptr };
};

class BitSet {
public:
    using Word = MecsU32;
    void set(const MecsAllocator& allocator, MecsSize slot, bool value);
    [[nodiscard]]
    bool test(MecsSize slot) const;

    void destroy(const MecsAllocator& allocator);
    void clear();

    [[nodiscard]]
    bool allZeroes() const;

    [[nodiscard]]
    bool contains(const BitSet& other) const;

    [[nodiscard]]
    BitSet clone(const MecsAllocator& allocator) const;

    [[nodiscard]]
    MecsSize count() const;

    template <typename F>
    void forEach(F&& func) const
    {
        MecsSize numBits = sizeof(Word) * 8 * mWords.count(); // NOLINT
        for (MecsSize i = 0; i < numBits; i++) {
            if (test(i)) {
                func(i);
            }
        }
    }

    bool operator==(const BitSet& other) const;
    bool operator!=(const BitSet& other) const = default;

private:
    MecsVec<Word> mWords;
};

template <typename T>
class GenArena {
public:
    struct EntryGeneration {
        MecsU32 generation : 31;
        bool taken         : 1;
    };
    struct Entry {
        T value;
        EntryGeneration tagGeneration;
    };

    void destroy(const MecsAllocator& allocator)
    {
        mEntries.destroy(allocator);
        mFreeIndices.destroy(allocator);
    }
    MecsSize count()
    {
        return mCount;
    }
    GenIndex push(const MecsAllocator& allocator, T value)
    {
        MecsSize index;
        TaggedGenIndex versIndex;
        versIndex.version.generation = 0;

        if (!mFreeIndices.empty()) {
            index = mFreeIndices.pop();
            Entry& entry = mEntries.at(index);
            MECS_ASSERT(!entry.tagGeneration.taken);
            versIndex.version.generation = entry.tagGeneration.generation;
            entry.tagGeneration.taken = true;
            entry.value = std::move(value);
        } else {
            index = mEntries.count();
            mEntries.push(allocator, Entry {
                                         std::move(value), { 0, true }
            });
        }
        versIndex.version.index = index;
        mCount += 1;
        return versIndex.index;
    }

    template <typename F>
    void forEach(F&& func)
    {
        MecsSize oldCount = mCount;
        MecsSize idx = 0;
        TaggedGenIndex genIndex;
        while (idx < mCount) {
            Entry* ent = &mEntries[idx];
            while (!ent->tagGeneration.taken) {
                idx++;
                MECS_ASSERT(idx < mCount);
                ent = &mEntries[idx];
            }

            genIndex.version.index = idx;
            genIndex.version.generation = ent->tagGeneration.generation;

            func(genIndex.index, ent->value);
            MECS_ASSERT(mCount == oldCount && "Element count changed during iteration! This is not allowed");
            idx++;
        }
    }

    T remove(const MecsAllocator& allocator, GenIndex index)
    {
        TaggedGenIndex versIndex;
        versIndex.index = index;

        Entry& entry = mEntries.at(versIndex.version.index);
        MECS_ASSERT(entry.tagGeneration.taken);
        MECS_ASSERT(entry.tagGeneration.generation == versIndex.version.generation);
        entry.tagGeneration.generation += 1;
        entry.tagGeneration.taken = false;
        T value = std::move(entry.value);
        entry.value.~T();

        mFreeIndices.push(allocator, versIndex.version.index);
        mCount--;
        return value;
    }

    T*
    at(GenIndex index)
    {

        TaggedGenIndex versIndex;
        versIndex.index = index;

        Entry& entry = mEntries.at(versIndex.version.index);
        MECS_ASSERT(entry.tagGeneration.taken);

        if (entry.tagGeneration.generation != versIndex.version.generation) {
            return nullptr;
        }

        return &entry.value;
    }

private:
    MecsVec<Entry> mEntries;
    MecsVec<MecsSize> mFreeIndices;
    MecsSize mCount { 0 };
};

template <typename V>
class SparseSet {
public:
    struct Key {
        MecsSize index;
        V v;
    };
    void insert(const MecsAllocator& allocator, const MecsSize entry, const V& value)
    {
        if (mSparse.count() <= entry) {
            mSparse.ensureSize(allocator, entry + 1);
        }

        MecsSize index = mDense.count();
        mDense.push(allocator, { entry, value });
        mSparse[entry] = index;
    }

    bool contains(const MecsSize entry) const
    {
        return mSparse.count() > entry
            && mSparse[entry] < mDense.count()
            && mDense[mSparse[entry]].index == entry;
    }

    bool remove(MecsSize entry)
    {
        if (!contains(entry)) {
            return false;
        }

        const MecsSize lastIndex = mDense.back().index;
        std::swap(mDense[mSparse[entry]], mDense.back());
        std::swap(mSparse[lastIndex], mSparse[entry]);
        mDense.pop();
        return true;
    }

    V& get(const MecsSize entry)
    {
        MECS_ASSERT(contains(entry));
        return mDense[mSparse[entry]].v;
    }
    const V& get(const MecsSize entry) const
    {
        MECS_ASSERT(contains(entry));
        return mDense[mSparse[entry]].v;
    }

    V& operator[](const MecsSize entry)
    {
        MECS_ASSERT(entry < mDense.count());
        return mDense[entry].v;
    }

    const V& operator[](const MecsSize entry) const
    {
        MECS_ASSERT(entry < mDense.count());
        return mDense[entry].v;
    }
    void destroy(const MecsAllocator& allocator)
    {
        mSparse.destroy(allocator);
        mDense.destroy(allocator);
    }
    [[nodiscard]]
    MecsSize count() const
    {
        return mDense.count();
    }

private:
    MecsVec<MecsSize> mSparse;
    MecsVec<Key> mDense {};
};
