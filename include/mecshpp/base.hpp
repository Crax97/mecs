#pragma once

#include "mecs/base.h"
#include "mecs/mecs.h"

#include <compare>
#include <sstream>

#define MECS_CONSTRUCTORS(Type)            \
    Type() = default;                      \
    Type(const Type&) = delete;            \
    Type& operator=(const Type&) = delete; \
                                           \
    Type(Type&& rhs) noexcept              \
        : mHandle(rhs.mHandle)             \
    {                                      \
        rhs.mHandle = nullptr;             \
    }                                      \
    Type& operator=(Type&& rhs) noexcept   \
    {                                      \
        mHandle = rhs.mHandle;             \
        rhs.mHandle = nullptr;             \
        return *this;                      \
    }

#define DEFINE_ID(Struct)                                                                          \
    namespace mecs {                                                                               \
    struct Struct {                                                                                \
        MecsU32 mID { MECS_INVALID };                                                              \
        constexpr auto operator<=>(const Struct& other) const = default;                           \
        constexpr bool operator==(const Struct& other) const = default;                            \
        constexpr static Struct invalid() { return { MECS_INVALID }; }                             \
        constexpr bool isValid() const { return mID != MECS_INVALID; }                             \
        constexpr bool isNull() const { return mID == MECS_INVALID; }                              \
        constexpr MecsU32 id() const { return mID; }                                               \
    };                                                                                             \
    }                                                                                              \
    namespace std {                                                                                \
    template <>                                                                                    \
    struct hash<mecs::Struct> {                                                                    \
        size_t operator()(const mecs::Struct& mID) const { return std::hash<MecsU32>()(mID.mID); } \
    };                                                                                             \
    template <>                                                                                    \
    struct formatter<mecs::Struct, char> {                                                         \
        constexpr auto parse(format_parse_context& ctx)                                            \
        {                                                                                          \
            return std::formatter<MecsU32>().parse(ctx);                                           \
        }                                                                                          \
        auto format(mecs::Struct& strukt, format_context& ctx) const                               \
        {                                                                                          \
            std::ostringstream out;                                                                \
            out << #Struct << " {" << strukt.id() << "}";                                          \
            return std::ranges::copy(std::move(out).str(), ctx.out()).out;                         \
        }                                                                                          \
    };                                                                                             \
    }

DEFINE_ID(ComponentID);
DEFINE_ID(PrefabID);
DEFINE_ID(EntityID);
