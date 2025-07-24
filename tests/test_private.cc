#include "test_private.hpp"
#include "mecs/base.h"
#include "private.h"

#include <cstdio>
#include <print>
#include <stacktrace>
#include <unordered_map>

#ifdef MECS_TESTS_LEAK_DETECTION
struct AllocInfo {
    std::stacktrace stack;
    MecsSize size;
    MecsSize align;
};

thread_local std::unordered_map<void*, AllocInfo> memAllocations;

AllocInfo getAllocInfo(MecsSize size, MecsSize align)
{
    AllocInfo info {};
    info.stack = std::stacktrace::current();
    info.size = size;
    info.align = align;
    return info;
}

void* debugMemalloc(void* userData, MecsSize size, MecsSize align)
{
    void* alloc = kDefaultAllocator.memAlloc(userData, size, align);
    memAllocations[alloc] = getAllocInfo(size, align);
    return alloc;
}
void* debugMemRealloc(void* userData, void* old, MecsSize oldSize, MecsSize align,
    MecsSize newSize)
{
    if (old != nullptr) {
        size_t removed = memAllocations.erase(old);
        MECS_ASSERT(removed > 0);
    }
    void* newAlloc = kDefaultAllocator.memRealloc(userData, old, oldSize, align, newSize);
    memAllocations[newAlloc] = getAllocInfo(newSize, align);
    return newAlloc;
}
void debugMemFree(void* userData, void* ptr)
{
    kDefaultAllocator.memFree(userData, ptr);
    if (ptr == nullptr) {
        return;
    }
    size_t removed = memAllocations.erase(ptr);
    MECS_ASSERT(removed > 0);
}

void debugAllocatorInit()
{
    std::println("Runing tests with memory leak detection enabled");
}

void printAllocInfo(void* addr, const AllocInfo& alloc)
{
    std::println(stderr, "---------------------------");
    std::println(stderr, "Memory leak at {} (lost {} bytes)", addr, alloc.size);
    for (const auto& stack : alloc.stack) {
        std::println(stderr, "\t{} -> {}:{} ", stack.description(), stack.source_file(), stack.source_line());
    }
};

void debugAllocatorCheck()
{
    for (const auto& alloc : memAllocations) {
        printAllocInfo(alloc.first, alloc.second);
    }
    if (!memAllocations.empty()) {
        std::println(stderr, "MEMORY LEAK DETECTED: {} allocations were not freed", memAllocations.size());
        MECS_ASSERT(false);
    } else {
        std::println(stderr, "No memory leaks detected :)");
    }
}
#endif

#if defined(CATCH_CONFIG_WCHAR) && defined(CATCH_PLATFORM_WINDOWS) && defined(_UNICODE) && !defined(DO_NOT_USE_WMAIN)
// Standard C/C++ Win32 Unicode wmain entry point
extern "C" int __cdecl wmain(int argc, wchar_t* argv[], wchar_t*[])
{
#else
// Standard C/C++ main entry point
int main(int argc, char* argv[])
{
#endif

#ifdef MECS_TESTS_LEAK_DETECTION
    debugAllocatorInit();
#endif
    int result = Catch::Session().run(argc, argv);

#ifdef MECS_TESTS_LEAK_DETECTION
    if (result == 0) {
        debugAllocatorCheck();
    }
#endif
    return result;
}
