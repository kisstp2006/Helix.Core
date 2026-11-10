#include <Helix/Core/Basics/Assert.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace helix
{
    namespace
    {
        AssertAction DefaultHandler(const char* expression, const char* message, const char* file, int line)
        {
            std::fprintf(stderr, "%s(%d): assertion failed: %s%s%s\n", file, line, expression, message ? " | " : "", message ? message : "");
            std::fflush(stderr);
            return HX_DEBUG ? AssertAction::Break : AssertAction::Abort;
        }

        std::atomic<AssertHandler> handler{&DefaultHandler};
    }

    AssertHandler SetAssertHandler(AssertHandler newHandler)
    {
        return handler.exchange(newHandler ? newHandler : &DefaultHandler);
    }

    AssertAction ReportAssert(const char* expression, const char* message, const char* file, int line)
    {
        return handler.load()(expression, message, file, line);
    }

    void Fatal(const char* message, const char* file, int line)
    {
        std::fprintf(stderr, "%s(%d): fatal: %s\n", file, line, message);
        std::fflush(stderr);
        AbortProcess();
    }

    void AbortProcess() { std::abort(); }
}
