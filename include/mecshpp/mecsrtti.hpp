#pragma once

#include "mecs/base.h"
#include <array>
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#if defined(MECS_COMPILER_MSVC)
#define MECS_RTTI_NO_INVALIDOFFSETOF_BEGIN
#define MECS_RTTI_NO_INVALIDOFFSETOF_END
#elif defined(MECS_COMPILER_GCC) or defined(MECS_COMPILER_CLANG)
#define MECS_RTTI_NO_INVALIDOFFSETOF_BEGIN \
    _Pragma("GCC diagnostic push")         \
        _Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")

#define MECS_RTTI_NO_INVALIDOFFSETOF_END \
    _Pragma("GCC diagnostic pop")
#else
#message "Unknown compiler, compilation may fail"
#define MECS_RTTI_NO_INVALIDOFFSETOF_BEGIN
#define MECS_RTTI_NO_INVALIDOFFSETOF_END
#endif

#define MECS_RTTI_FRIEND(Type) \
    friend struct ::mecs::RTTI<Type>;

#define MECS_RTTI_STRUCT_BEGIN(Type)                                                           \
    MECS_RTTI_NO_INVALIDOFFSETOF_BEGIN                                                         \
    template <>                                                                                \
    struct ::mecs::RTTI<Type> {                                                                \
        using MecsCurrentT = Type;                                                             \
        constexpr static MecsTypeID kTypeID = ::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1); \
        constexpr static MecsSize kSizeOf = sizeof(Type);                                      \
        constexpr static MecsSize kAlignOf = alignof(Type);                                    \
        static const ComponentInfo& componentInfo()                                            \
        {                                                                                      \
            static const char* kName = #Type;                                                  \
            static std::array kMembers = ::mecs::detail::makeArray<MecsComponentMember>(0

#define MECS_RTTI_STRUCT_MEMBER(Name) \
    , MecsComponentMember { .typeID = ::mecs::typeIdOf<decltype(MecsCurrentT::Name)>(), .offset = offsetof(MecsCurrentT, Name), .name = #Name }

#define MECS_RTTI_STRUCT_END()                            \
        );                                                \
    static ComponentInfo gInfo {                          \
        .name = kName,                                    \
        .typeID = kTypeID,                                \
        .size = sizeof(MecsCurrentT),                     \
        .align = alignof(MecsCurrentT),                   \
        .memberCount = kMembers.size(),                   \
        .members = kMembers.data(),                       \
        .init = ::mecs::detail::init<MecsCurrentT>,       \
        .copy = ::mecs::detail::copy<MecsCurrentT>,       \
        .destroy = ::mecs::detail::destroy<MecsCurrentT>, \
    };                                                    \
    return gInfo;                                         \
    }                                                     \
    }                                                     \
    ;                                                     \
    MECS_RTTI_NO_INVALIDOFFSETOF_END

#define MECS_RTTI_SIMPLE(Type)   \
    MECS_RTTI_STRUCT_BEGIN(Type) \
    MECS_RTTI_STRUCT_END()

namespace mecs {
namespace detail {

    template <typename T>
    static void init(void* ptr)
    {
        new (ptr) T();
    }

    template <typename T>
    static void copy(const void* src, void* dest, [[maybe_unused]] MecsSize size)
    {
        T* tDest = reinterpret_cast<T*>(dest);
        const T* tSrc = reinterpret_cast<const T*>(src);
        *tDest = *tSrc;
    }

    template <typename T>
    static void destroy(void* ptr)
    {
        T* tPtr = reinterpret_cast<T*>(ptr);
        tPtr->~T();
    }
    template <typename T, typename... Args>
    constexpr auto makeArray(int unused, Args... args)
    {
        return std::array<T, sizeof...(Args)> { { args... } };
    }
    constexpr uint64_t fnv1a(const char* str, size_t n, uint64_t basis = 14695981039346656037U) // NOLINT
    {
        return n == 0 ? basis : fnv1a(str + 1, n - 1, (basis ^ str[0]) * 1099511628211U); // NOLINT
    }

}
template <typename T>
struct RTTI {
};

template <typename T>
using rttiOf = mecs::RTTI<std::remove_const_t<std::remove_reference_t<T>>>;

template <typename T>
concept HasRTTI = requires {
    { mecs::rttiOf<T>::componentInfo() } -> std::convertible_to<ComponentInfo>;
};

template <typename T>
constexpr MecsTypeID typeIdOf()
    requires(HasRTTI<T>)
{
    return rttiOf<T>::kTypeID;
}
}

MECS_RTTI_SIMPLE(int);
MECS_RTTI_SIMPLE(unsigned int);
MECS_RTTI_SIMPLE(char);
MECS_RTTI_SIMPLE(float);
MECS_RTTI_SIMPLE(double);
MECS_RTTI_SIMPLE(long);
MECS_RTTI_SIMPLE(std::string);

static_assert(mecs::HasRTTI<int>);

static_assert(mecs::typeIdOf<int>() != mecs::typeIdOf<unsigned int>());
