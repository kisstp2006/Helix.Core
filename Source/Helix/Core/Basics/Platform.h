#pragma once

#if defined(__EMSCRIPTEN__)
#define HX_PLATFORM_WEB 1
#elif defined(__ANDROID__)
#define HX_PLATFORM_ANDROID 1
#elif defined(_WIN32)
#define HX_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define HX_PLATFORM_LINUX 1
#else
#error "Helix: unsupported platform"
#endif

#ifndef HX_PLATFORM_WEB
#define HX_PLATFORM_WEB 0
#endif
#ifndef HX_PLATFORM_ANDROID
#define HX_PLATFORM_ANDROID 0
#endif
#ifndef HX_PLATFORM_WINDOWS
#define HX_PLATFORM_WINDOWS 0
#endif
#ifndef HX_PLATFORM_LINUX
#define HX_PLATFORM_LINUX 0
#endif

#define HX_PLATFORM_POSIX (HX_PLATFORM_LINUX || HX_PLATFORM_ANDROID || HX_PLATFORM_WEB)

#if defined(__clang__)
#define HX_COMPILER_CLANG 1
#define HX_COMPILER_MSVC 0
#elif defined(_MSC_VER)
#define HX_COMPILER_CLANG 0
#define HX_COMPILER_MSVC 1
#else
#error "Helix: unsupported compiler"
#endif

#if defined(NDEBUG)
#define HX_DEBUG 0
#else
#define HX_DEBUG 1
#endif

#if HX_COMPILER_MSVC
#define HX_FORCE_INLINE __forceinline
#define HX_NO_INLINE __declspec(noinline)
#define HX_DEBUG_BREAK() __debugbreak()
#define HX_LIKELY(x) (x)
#define HX_UNLIKELY(x) (x)
#else
#define HX_FORCE_INLINE inline __attribute__((always_inline))
#define HX_NO_INLINE __attribute__((noinline))
#if HX_PLATFORM_WEB
#define HX_DEBUG_BREAK() __builtin_trap()
#else
#define HX_DEBUG_BREAK() __builtin_debugtrap()
#endif
#define HX_LIKELY(x) __builtin_expect(!!(x), 1)
#define HX_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

// Global data the headers read, in the shared engine library (HelixEngine): code built against that library
// (HX_SHARED_IMPORT: the player, game modules) imports it on Windows; the libraries' own code defines it.
#if HX_PLATFORM_WINDOWS && defined(HX_SHARED_IMPORT)
#define HX_IMPORT_DATA __declspec(dllimport)
#else
#define HX_IMPORT_DATA
#endif
