#pragma once

#include "mecs/base.h"
#include "mecshpp/mecsrtti.hpp"
#include <array>
#include <concepts>
#include <cstdint>
#include <format>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define MECS_RTTI_FRIEND(Type) \
    friend struct mecs::RTTIDefine<Type>;

#define MECS_RTTI_STRUCT_PRELUDE(Type, Name, Kind)                                            \
    template <>                                                                               \
    struct mecs::ConcreteType<::mecs::detail::fnv1a(Name, sizeof(Name) - 1)> {                \
        using kConcrete = Type;                                                               \
    };                                                                                        \
    template <>                                                                               \
    struct mecs::RTTIDefine<Type> {                                                           \
        using MecsCurrentT = Type;                                                            \
        constexpr static ::mecs::RttiKind kKind = Kind;                                       \
        inline static ::mecs::TypeID kTypeID = ::mecs::detail::fnv1a(Name, sizeof(Name) - 1); \
        constexpr static MecsSize kSizeOf = sizeof(Type);                                     \
        constexpr static MecsSize kAlignOf = alignof(Type);                                   \
                                                                                              \
        constexpr static const char* kName = Name;                                            \
        static const ::mecs::RTTI& get()                                                      \
        {                                                                                     \
            static auto membersContainer = mecs::detail::MemberContainerZero { }

#define MECS_RTTI_STRUCT_BEGIN(Type) \
    MECS_RTTI_STRUCT_PRELUDE(Type, #Type, ::mecs::RttiKind::eStruct)

// NOLINTBEGIN the + is required for syntax masturbation
#define MECS_RTTI_STRUCT_MEMBER(Name) \
    +::mecs::Member { .name = #Name, .memberRtti = &::mecs::rttiOf<decltype(MecsCurrentT::Name)>(), .getMemberFn = ::mecs::detail::GetMember<decltype(MecsCurrentT::Name) MecsCurrentT::*, &MecsCurrentT::Name>::get }
// NOLINTEND

// NOLINTBEGIN the + is required for syntax masturbation
#define MECS_RTTI_STRUCT_METHOD(Name) \
    +::mecs::detail::GetMethod<decltype(&MecsCurrentT::Name), &MecsCurrentT::Name>::getMethod(#Name)
// NOLINTEND
#define MECS_RTTI_STRUCT_END()                                                                \
    ;                                                                                         \
    static ::mecs::RTTIStruct rtti = []() {                                                   \
        ::mecs::RTTIStruct rttiI;                                                             \
        rttiI.name = kName;                                                                   \
        rttiI.typeID = kTypeID;                                                               \
        rttiI.kind = RttiKind::eStruct;                                                       \
        rttiI.size = sizeof(MecsCurrentT);                                                    \
        rttiI.align = alignof(MecsCurrentT);                                                  \
        rttiI.init = ::mecs::detail::init<MecsCurrentT>;                                      \
        rttiI.copy = ::mecs::detail::copy<MecsCurrentT>;                                      \
        rttiI.destroy = ::mecs::detail::destroy<MecsCurrentT>;                                \
        rttiI.members = { membersContainer.members.begin(), membersContainer.members.end() }; \
        rttiI.methods = { membersContainer.methods.begin(), membersContainer.methods.end() }; \
        return rttiI;                                                                         \
    }();                                                                                      \
    return rtti;                                                                              \
    }                                                                                         \
    }                                                                                         \
    ;

#define MECS_RTTI_ENUM_BEGIN(Type)                                                              \
    template <>                                                                                 \
    struct mecs::ConcreteType<::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1)> {                \
        using kConcrete = Type;                                                                 \
    };                                                                                          \
    template <>                                                                                 \
    struct mecs::RTTIDefine<Type> {                                                             \
        using MecsCurrentT = Type;                                                              \
        inline static ::mecs::TypeID kTypeID = ::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1); \
        constexpr static MecsSize kSizeOf = sizeof(Type);                                       \
        constexpr static MecsSize kAlignOf = alignof(Type);                                     \
                                                                                                \
        constexpr static const char* kName = #Type;                                             \
        using UnderlyingType = std::underlying_type_t<Type>;                                    \
        constexpr static auto variantsContainer = ::mecs::detail::VariantContainer<0> { }

#define MECS_RTTI_ENUM_VARIANT(VariantValue, Name) \
    +::mecs::Variant { .name = Name, .value = static_cast<MecsU32>(MecsCurrentT::VariantValue) }
#define MECS_RTTI_ENUM_END()                                                                                                 \
    ;                                                                                                                        \
    constexpr static auto kVariants = variantsContainer.variants;                                                            \
    constexpr static MecsSize kNumVariants = kVariants.size();                                                               \
    static const ::mecs::RTTI& get()                                                                                         \
    {                                                                                                                        \
        static ::mecs::RTTIEnum rtti = []() {                                                                                \
            ::mecs::RTTIEnum rttiI;                                                                                          \
            rttiI.name = kName;                                                                                              \
            rttiI.typeID = kTypeID;                                                                                          \
            rttiI.kind = RttiKind::eEnum;                                                                                    \
            rttiI.size = sizeof(MecsCurrentT);                                                                               \
            rttiI.align = alignof(MecsCurrentT);                                                                             \
            rttiI.init = ::mecs::detail::init<MecsCurrentT>;                                                                 \
            rttiI.copy = ::mecs::detail::copy<MecsCurrentT>;                                                                 \
            rttiI.destroy = ::mecs::detail::destroy<MecsCurrentT>;                                                           \
            rttiI.variants = { kVariants.begin(), kVariants.end() };                                                         \
            rttiI.get = [](void* ptr) { return static_cast<MecsU32>(*static_cast<MecsCurrentT*>(ptr)); };                    \
            rttiI.set = [](void* ptr, MecsU32 nval) { *static_cast<MecsCurrentT*>(ptr) = static_cast<MecsCurrentT>(nval); }; \
            return rttiI;                                                                                                    \
        }();                                                                                                                 \
        return rtti;                                                                                                         \
    }                                                                                                                        \
    }                                                                                                                        \
    ;

#define MECS_RTTI_SIMPLE(Type)   \
    MECS_RTTI_STRUCT_BEGIN(Type) \
    MECS_RTTI_STRUCT_END()

#define MECS_RTTI_FIELD(Type)                                                                   \
    template <>                                                                                 \
    struct mecs::RTTIDefine<Type> {                                                             \
        using MecsCurrentT = Type;                                                              \
        inline static ::mecs::TypeID kTypeID = ::mecs::detail::fnv1a(#Type, sizeof(#Type) - 1); \
        constexpr static MecsSize kSizeOf = sizeof(Type);                                       \
        constexpr static MecsSize kAlignOf = alignof(Type);                                     \
                                                                                                \
        constexpr static const char* kName = #Type;                                             \
        static const ::mecs::RTTI& get()                                                        \
        {                                                                                       \
            static ::mecs::RTTI rtti = []() {                                                   \
                ::mecs::RTTI rttiI;                                                             \
                rttiI.name = kName;                                                             \
                rttiI.typeID = kTypeID;                                                         \
                rttiI.kind = RttiKind::eField;                                                  \
                rttiI.size = sizeof(MecsCurrentT);                                              \
                rttiI.align = alignof(MecsCurrentT);                                            \
                rttiI.init = ::mecs::detail::init<MecsCurrentT>;                                \
                rttiI.copy = ::mecs::detail::copy<MecsCurrentT>;                                \
                rttiI.destroy = ::mecs::detail::destroy<MecsCurrentT>;                          \
                return rttiI;                                                                   \
            }();                                                                                \
            return rtti;                                                                        \
        }                                                                                       \
    }

namespace mecs {
using TypeID = uint64_t;

enum class RttiKind : MecsU8 {
    eStruct,
    eEnum,
    eField,
    eVec,
};

using GetMemberFn = void* (*)(void*);
using CallMethodFn = void (*)(void* self, void* returnAddr, void** args);
struct Member {
    const char* name;
    const struct RTTI* memberRtti;
    GetMemberFn getMemberFn;
};
constexpr size_t kMaxMethodArguments = 6;
struct Method {
    const char* name;
    TypeID returnType;
    std::array<const struct RTTI*, kMaxMethodArguments> arguments;
    MecsSize arity;
    CallMethodFn call;
};

template <typename T>
struct TVariant {
    const char* name;
    T value;
};

struct Variant {
    const char* name;
    MecsU32 value;
};

struct RTTI {
    const char* name;
    mecs::TypeID typeID;
    RttiKind kind;
    MecsSize size;
    MecsSize align;
    PFNMecsComponentInit init;
    PFNMecsComponentCopy copy;
    PFNMecsComponentDestroy destroy;
};

struct RTTIStruct : public RTTI {
    std::vector<Member> members;
    std::vector<Method> methods;
};

using EnumGetFn = MecsU32 (*)(void*);
using EnumSetFn = void (*)(void*, MecsU32);

struct RTTIEnum : public RTTI {
    std::vector<Variant> variants;
    EnumGetFn get;
    EnumSetFn set;
};

using VecGetCountFn = MecsSize (*)(const void*);
using VecGetElemFn = void* (*)(void*, MecsSize);
using VecClearFn = void (*)(void*);
using VecPushBackFn = void (*)(void*, void*);
using VecPopBackFn = void (*)(void*, void*);

struct RTTIVec : public RTTI {
    const struct RTTI* elementRtti;
    VecGetCountFn getCount;
    VecGetElemFn getElem;
    VecClearFn clear;
    VecPushBackFn pushBack;
    VecPopBackFn popBack;
};

template <typename T>
struct RTTIDefine {
};
template <mecs::TypeID typeID>
struct ConcreteType {
};

template <mecs::TypeID typeID>
using concreteType = typename ConcreteType<typeID>::kConcrete;

template <typename T>
const RTTI& rttiOf()
{
    return RTTIDefine<std::remove_const_t<std::remove_reference_t<T>>>::get();
};

template <typename T>
concept HasRTTI = requires {
    { mecs::RTTIDefine<T>::get() } -> std::convertible_to<const RTTI&>;
};

template <typename T>
mecs::TypeID typeIdOf()
    requires(HasRTTI<T>)
{
    return rttiOf<T>().typeID;
}
namespace detail {

    template <typename T, std::size_t... I>
    constexpr std::array<T, sizeof...(I) + 1> concat(const std::array<T, sizeof...(I)>& left, const T& rhs, std::index_sequence<I...> idx)
    {
        return { left[I]..., rhs };
    }

    template <MecsSize nMembers, MecsSize nMethods>
    struct MemberContainer {
        std::array<Member, nMembers> members;
        std::array<Method, nMethods> methods;

        constexpr MemberContainer<nMembers + 1, nMethods> operator+(const Member& mem)
        {
            return MemberContainer<nMembers + 1, nMethods> { .members = concat(members, mem, std::make_index_sequence<nMembers>()), .methods = methods };
        }
        constexpr MemberContainer<nMembers, nMethods + 1> operator+(const Method& meth)
        {
            return MemberContainer<nMembers, nMethods + 1> { .members = members, .methods = concat(methods, meth, std::make_index_sequence<nMethods>()) };
        }
    };

    using MemberContainerZero = MemberContainer<0, 0>;

    template <MecsSize nMembers>
    struct VariantContainer {
        std::array<Variant, nMembers> variants;

        constexpr VariantContainer<nMembers + 1> operator+(const Variant& var)
        {
            return VariantContainer<nMembers + 1> { .variants = concat(variants, var, std::make_index_sequence<nMembers>()) };
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

    template <typename T, typename R, typename... Args, size_t... I>
    R callHelper(T* ptr, R (T::*methodPtr)(Args...), void** args, std::index_sequence<I...> idx)
    {
        if constexpr (std::is_same_v<R, void>) {
            (*ptr.*methodPtr)(std::forward<Args>(*static_cast<Args*>(args[I]))...);
        } else {
            return (*ptr.*methodPtr)(std::forward<Args>(*static_cast<Args*>(args[I]))...);
        }
    }

    template <typename R>
    constexpr TypeID returnTypeIdOf()
    {
        return typeIdOf<R>();
    }
    template <>
    constexpr TypeID returnTypeIdOf<void>()
    {
        return MECS_INVALID;
    }

    template <typename T, T>
    struct GetMethod;
    template <typename T, typename R, typename... Args, R (T::*methodPtr)(Args...)>
    struct GetMethod<R (T::*)(Args...), methodPtr> {
        using Return = R;
        constexpr static MecsSize kArity = sizeof...(Args);
        constexpr static mecs::TypeID kReturnTypeID = returnTypeIdOf<R>();
        constexpr static std::array<const RTTI*, kMaxMethodArguments> kArgTypes = { { rttiOf<Args>()... } };
        static void call(void* self, void* returnAddr, void** args)
        {
            T* tBase = static_cast<T*>(self);
            if constexpr (std::is_same_v<R, void>) {
                callHelper(tBase, methodPtr, args, std::make_index_sequence<sizeof...(Args)>());
            } else {
                R ret = callHelper(tBase, methodPtr, args, std::make_index_sequence<sizeof...(Args)>());
                R* retPtr = static_cast<R*>(returnAddr);
                *retPtr = std::move(ret);
            }
        }

        constexpr static Method getMethod(const char* name)
        {
            return {
                .name = name,
                .returnType = kReturnTypeID,
                .arguments = kArgTypes,
                .arity = kArity,
                .call = call
            };
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
struct RTTIDefine<std::vector<T>> {
    constexpr static ::mecs::RttiKind kKind = mecs::RttiKind::eVec;
    inline static mecs::TypeID kTypeID = mecs::typeIdOf<T>();

    constexpr static MecsSize getSize(const void* ptr)
    {
        const std::vector<T>& vec = *static_cast<const std::vector<T>*>(ptr);
        return vec.size();
    }
    constexpr static void* at(void* ptr, MecsSize index)
    {
        std::vector<T>& vec = *static_cast<std::vector<T>*>(ptr);
        return &vec.at(index);
    }
    constexpr static void pushBack(void* ptr, void* outer)
    {

        std::vector<T>& vec = *static_cast<std::vector<T>*>(ptr);
        if (outer != nullptr) {
            vec.emplace_back(std::move(*static_cast<T*>(outer)));
        } else {
            vec.emplace_back();
        }
    }
    constexpr static void popBack(void* ptr, void* outer)
    {
        std::vector<T>& vec = *static_cast<std::vector<T>*>(ptr);

        if (outer != nullptr) {
            *static_cast<T*>(outer) = std::move(vec.back());
        }
        vec.pop_back();
    }
    constexpr static void clear(void* ptr)
    {
        std::vector<T>& vec = *static_cast<std::vector<T>*>(ptr);
        vec.clear();
    }
    static const ::mecs::RTTI& get()
    {
        static ::mecs::RTTIVec gRtti = []() {
            ::mecs::RTTIVec res;
            const auto& childRtti = rttiOf<T>();
            static std::string gVectorFormat = std::format("std::vector<{}>", childRtti.name);
            res.name = gVectorFormat.c_str();
            res.typeID = detail::fnv1a(gVectorFormat.c_str(), gVectorFormat.size() - 1);
            res.kind = RttiKind::eVec;
            res.size = sizeof(std::vector<T>);
            res.align = alignof(std::vector<T>);
            res.init = ::mecs::detail::init<std::vector<T>>;
            res.copy = ::mecs::detail::copy<std::vector<T>>;
            res.destroy = ::mecs::detail::destroy<std::vector<T>>;

            res.elementRtti = &childRtti;
            res.getCount = getSize;
            res.getElem = at;
            res.clear = clear;
            res.pushBack = pushBack;
            res.popBack = popBack;

            return res;
        }();
        return gRtti;
    }
};

template <auto Start, auto End, auto Inc, class F>
constexpr void constexprFor(F&& func)
{
    if constexpr (Start < End) {
        func(std::integral_constant<decltype(Start), Start>());
        constexprFor<Start + Inc, End, Inc>(func);
    }
}

/*Useful for iterating at compile time on the members of a struct, e.g
```

template <typename T>
    requires(mecs::HasRTTI<T>)
std::string dump(T& val) {
std::string res;

using rtti = mecs::rttiOf<T>;
constexpr auto kMembers = rtti::kMembers;

mecs::constexprFor(kMembers, [&kMembers, &res, &val](auto idx) {
    constexpr auto kMember = kMembers[idx];
    using memTy = mecs::concreteType<kMember.typeID>;
    memTy& mem = *static_cast<memTy*>(kMember.getMemberFn(&val));
    res += kMember.name;
    res += " ";
    res += dump<memTy>(mem);
});
return res;
}
```
*/
template <typename F, typename Tuple>
constexpr void constexprFor(Tuple&& tuple, F&& func)
{
    constexpr size_t kCount = std::tuple_size_v<std::decay_t<Tuple>>;
    constexprFor<size_t(0), kCount, 1>([&](auto idx) { func(idx); });
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
