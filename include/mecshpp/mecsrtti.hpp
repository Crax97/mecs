#pragma once

#include "mecs/base.h"
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

#define MECS_RTTI(Type)                                                        \
    template <>                                                                \
    struct ::mecs::RTTI<Type> {                                                \
        constexpr static MecsTypeID kTypeID = fnv1a(#Type, sizeof(#Type) - 1); \
    }

namespace mecs {
constexpr uint64_t fnv1a(const char* str, size_t n, uint64_t basis = 14695981039346656037U) // NOLINT
{
    return n == 0 ? basis : fnv1a(str + 1, n - 1, (basis ^ str[0]) * 1099511628211U); // NOLINT
}
template <typename T>
struct RTTI {
};

template <typename T>
using rttiOf = mecs::RTTI<std::remove_const_t<std::remove_reference_t<T>>>;

template <typename T>
concept HasRTTI = requires {
    { mecs::rttiOf<T>::kTypeID } -> std::convertible_to<MecsTypeID>;
};
}

MECS_RTTI(int);
MECS_RTTI(unsigned int);
MECS_RTTI(char);
MECS_RTTI(float);
MECS_RTTI(double);
MECS_RTTI(long);
MECS_RTTI(std::string);

static_assert(mecs::HasRTTI<int>);

static_assert(mecs::rttiOf<int>::kTypeID != mecs::rttiOf<unsigned int>::kTypeID);
