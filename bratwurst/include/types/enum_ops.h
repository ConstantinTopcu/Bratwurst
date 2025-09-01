#pragma once

#include "core.h"

#include <type_traits>

#define ENABLE_ENUM_ARITHMETIC(EnumType) \
inline EnumType& operator++(EnumType& e) { \
    e = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(e) + 1); \
    return e; \
} \
inline EnumType operator++(EnumType& e, int) { \
    EnumType old = e; \
    ++e; \
    return old; \
} \
inline EnumType& operator--(EnumType& e) { \
    e = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(e) - 1); \
    return e; \
} \
inline EnumType operator--(EnumType& e, int) { \
    EnumType old = e; \
    --e; \
    return old; \
}
