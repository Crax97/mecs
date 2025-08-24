#pragma once

#include "mecs/base.h"
#include <array>
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define MECS_RTTI_FRIEND(Type) \
    friend struct ::mecs::RTTI<Type>;

#define MECS_RTTI_STRUCT_PRELUDE(Type, Name, TyID, Kind)    \
    template <>                                             \
    struct ::mecs::RTTI<Type> {                             \
        using MecsCurrentT = Type;                          \
        constexpr static ::mecs::RttiKind kKind = Kind;     \
        constexpr static ::mecs::TypeID kTypeID = TyID;     \
        constexpr static MecsSize kSizeOf = sizeof(Type);   \
        constexpr static MecsSize kAlignOf = alignof(Type); \
                                                            \
        constexpr static const char* kName = Name;

#define MECS_RTTI_STRUCT_BEGIN(Type)                                                                                  \
    MECS_RTTI_STRUCT_PRELUDE(Type, #Type, ::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1), ::mecs::RttiKind::eStruct) \
    constexpr static auto membersContainer = ::mecs::detail::MemberContainer<0> { }

// NOLINTBEGIN the + is required for syntax masturbation
#define MECS_RTTI_STRUCT_MEMBER(Name) \
    +::mecs::Member { .name = #Name, .typeID = ::mecs::typeIdOf<decltype(MecsCurrentT::Name)>(), .getMemberFn = ::mecs::detail::GetMember<decltype(MecsCurrentT::Name) MecsCurrentT::*, &MecsCurrentT::Name>::get }
// NOLINTEND

#define MECS_RTTI_STRUCT_END()                                 \
    ;                                                          \
    constexpr static auto kMembers = membersContainer.members; \
    static const ComponentInfo& componentInfo()                \
    {                                                          \
        static ComponentInfo gInfo {                           \
            .typeID = kTypeID,                                 \
            .name = kName,                                     \
            .size = sizeof(MecsCurrentT),                      \
            .align = alignof(MecsCurrentT),                    \
            .init = ::mecs::detail::init<MecsCurrentT>,        \
            .copy = ::mecs::detail::copy<MecsCurrentT>,        \
            .destroy = ::mecs::detail::destroy<MecsCurrentT>,  \
        };                                                     \
        return gInfo;                                          \
    }                                                          \
    }                                                          \
    ;

#define MECS_RTTI_ENUM_BEGIN(Type)                                                                                   \
    MECS_RTTI_STRUCT_PRELUDE(Type, #Type, ::mecs::typeIdOf<std::underlying_type_t<Type>>(), ::mecs::RttiKind::eEnum) \
    using UnderlyingType = std::underlying_type_t<Type>;                                                             \
    constexpr static auto variantsContainer = ::mecs::detail::VariantContainer<UnderlyingType, 0> { }

#define MECS_RTTI_ENUM_VARIANT(VariantValue, Name) \
    +::mecs::Variant<UnderlyingType> { .name = Name, .value = static_cast<UnderlyingType>(MecsCurrentT::VariantValue) }
#define MECS_RTTI_ENUM_END()                                      \
    ;                                                             \
    constexpr static auto kVariants = variantsContainer.variants; \
    constexpr static MecsSize kNumVariants = kVariants.size();    \
    }                                                             \
    ;

#define MECS_RTTI_SIMPLE(Type)   \
    MECS_RTTI_STRUCT_BEGIN(Type) \
    MECS_RTTI_STRUCT_END()

#define MECS_RTTI_FIELD(Type)                                                                                        \
    MECS_RTTI_STRUCT_PRELUDE(Type, #Type, ::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1), ::mecs::RttiKind::eField) \
    constexpr static auto membersContainer = ::mecs::detail::MemberContainer<0> { }                                  \
    MECS_RTTI_STRUCT_END()

namespace mecs {
using TypeID = uint64_t;

enum class RttiKind : MecsU8 {
    eStruct,
    eEnum,
    eField,
};

using GetMemberFn = void* (*)(void*);
struct Member {
    const char* name;
    TypeID typeID;
    GetMemberFn getMemberFn;
};

template <typename T>
struct Variant {
    const char* name;
    T value;
};

namespace detail {

    template <typename T, std::size_t... I>
    constexpr std::array<T, sizeof...(I) + 1> concat(const std::array<T, sizeof...(I)>& left, const T& rhs, std::index_sequence<I...> idx)
    {
        return { left[I]..., rhs };
    }

    template <MecsSize nMembers>
    struct MemberContainer {
        std::array<Member, nMembers> members;

        constexpr MemberContainer<nMembers + 1> operator+(const Member& mem)
        {
            return MemberContainer<nMembers + 1> { .members = concat(members, mem, std::make_index_sequence<nMembers>()) };
        }
    };
    template <typename T, MecsSize nMembers>
    struct VariantContainer {
        std::array<Variant<T>, nMembers> variants;

        constexpr VariantContainer<T, nMembers + 1> operator+(const Variant<T>& var)
        {
            return VariantContainer<T, nMembers + 1> { .variants = concat(variants, var, std::make_index_sequence<nMembers>()) };
        }
    };

    template <typename T, T>
    struct GetMember;

    template <typename T, typename M, M T::* mPtr>
    struct GetMember<M T::*, mPtr> {
        static void* get(void* base)
        {
            T* tBase = static_cast<T*>(base);
            return &(tBase->*mPtr);
        }
    };

    template <typename T, typename M, T::M* mPtr>
    void* getMember(void* base)
    {
        T* tBase = static_cast<T*>(base);
        return &base.*mPtr;
    }

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
    { mecs::rttiOf<T>::kTypeID } -> std::convertible_to<mecs::TypeID>;
};

template <typename T>
constexpr mecs::TypeID typeIdOf()
    requires(HasRTTI<T>)
{
    return rttiOf<T>::kTypeID;
}
}

MECS_RTTI_FIELD(int);
MECS_RTTI_FIELD(unsigned int);
MECS_RTTI_FIELD(long);
MECS_RTTI_FIELD(unsigned long);
MECS_RTTI_FIELD(unsigned long long);
MECS_RTTI_FIELD(char);
MECS_RTTI_FIELD(unsigned char);
MECS_RTTI_FIELD(float);
MECS_RTTI_FIELD(double);
MECS_RTTI_FIELD(std::string);

static_assert(mecs::HasRTTI<int>);

static_assert(mecs::typeIdOf<int>() != mecs::typeIdOf<unsigned int>());
