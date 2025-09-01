#pragma once

#include <cstdint>

#ifdef _MSC_VER
#define COMPILER_MSVC
#elif defined(__GNUC__)
#define COMPILER_GCC
#elif defined(__clang__)
#define COMPILER_CLANG
#else
#error "Unsupported compiler"
#endif

#ifdef NDEBUG
#define BUILD_RELEASE
#else
#define BUILD_DEBUG
#endif

#ifdef _WIN32
#define PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLATFORM_LINUX
#elif defined(__APPLE__)
#define PLATFORM_MACOS
#else
#error "Unsupported platform"
#endif

#ifdef COMPILER_MSVC
#define DebugBreak() __debugbreak()
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#include <csignal>
#define DebugBreak() raise(SIGTRAP)
#endif

#ifdef BUILD_DEBUG
#include <iostream>

    // Helpers to count args
#define GET_MACRO(_1,_2,NAME,...) NAME

// ASSERT with just expr
#define ASSERT1(expr) \
        do { \
            if (!(expr)) { \
                DebugBreak(); \
                std::cerr << "Assertion failed: " << #expr << std::endl; \
                std::cerr << "In file: " << __FILE__ << ", line: " << __LINE__ << std::endl; \
            } \
        } while (0)

// ASSERT with expr + msg
#define ASSERT2(expr, msg) \
        do { \
            if (!(expr)) { \
                DebugBreak(); \
                std::cerr << "Assertion failed: " << #expr << ", message: " << msg << std::endl; \
                std::cerr << "In file: " << __FILE__ << ", line: " << __LINE__ << std::endl; \
            } \
        } while (0)

// Dispatcher macro
#define ASSERT(...) GET_MACRO(__VA_ARGS__, ASSERT2, ASSERT1)(__VA_ARGS__)

#else
#define ASSERT(...) ((void)0)
#endif

namespace Bratwurst
{
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using byte = uint8;
}