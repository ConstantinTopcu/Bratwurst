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

#ifdef ENABLE_DEBUG
#include <iostream>

#define ASSERT(expr) \
        do { \
            if (!(expr)) { \
                std::cerr << "[ASSERT FAILED] " << #expr \
                          << "\n  Location: " << __FILE__ << ":" << __LINE__ << std::endl; \
                DebugBreak(); \
            } \
        } while(0)

// Assert with custom message
#define ASSERT_MSG(expr, msg) \
        do { \
            if (!(expr)) { \
                std::cerr << "[ASSERT FAILED] " << #expr \
                          << "\n  Message: " << msg \
                          << "\n  Location: " << __FILE__ << ":" << __LINE__ << std::endl; \
                DebugBreak(); \
            } \
        } while(0)


#else
#define ASSERT(expr) ((void)0)
#define ASSERT_MSG(expr, msg) ((void)0)
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