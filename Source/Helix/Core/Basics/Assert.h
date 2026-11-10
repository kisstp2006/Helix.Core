#pragma once

#include <Helix/Core/Basics/Platform.h>

namespace helix
{
    enum class AssertAction
    {
        Break,
        Continue,
        Abort,
    };

    using AssertHandler = AssertAction (*)(const char* expression, const char* message, const char* file, int line);

    AssertHandler SetAssertHandler(AssertHandler handler);
    AssertAction ReportAssert(const char* expression, const char* message, const char* file, int line);
    [[noreturn]] void Fatal(const char* message, const char* file, int line);
    [[noreturn]] void AbortProcess();
}

#define HX_VERIFY_MSG(expr, message)                                                            \
    do                                                                                          \
    {                                                                                           \
        if (HX_UNLIKELY(!(expr)))                                                               \
        {                                                                                       \
            switch (::helix::ReportAssert(#expr, message, __FILE__, __LINE__))                  \
            {                                                                                   \
            case ::helix::AssertAction::Break: HX_DEBUG_BREAK(); break;                         \
            case ::helix::AssertAction::Abort: ::helix::AbortProcess();                         \
            case ::helix::AssertAction::Continue: break;                                        \
            }                                                                                   \
        }                                                                                       \
    } while (0)

#define HX_VERIFY(expr) HX_VERIFY_MSG(expr, nullptr)
#define HX_FATAL(message) ::helix::Fatal(message, __FILE__, __LINE__)

#if HX_DEBUG
#define HX_ASSERT(expr) HX_VERIFY(expr)
#define HX_ASSERT_MSG(expr, message) HX_VERIFY_MSG(expr, message)
#else
#define HX_ASSERT(expr) ((void)0)
#define HX_ASSERT_MSG(expr, message) ((void)0)
#endif
