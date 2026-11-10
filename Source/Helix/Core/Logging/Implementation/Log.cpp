#include <Helix/Core/Logging/Log.h>

#include <atomic>
#include <cstdio>
#include <mutex>

#if HX_PLATFORM_WINDOWS
#include <windows.h>
#elif HX_PLATFORM_ANDROID
#include <android/log.h>
#endif

namespace helix
{
    LogChannel CoreLog{"Core"};

    namespace
    {
        struct SinkEntry
        {
            LogSink sink;
            void* user;
        };

        constexpr usize kMaxSinks = 8;
        SinkEntry sinks[kMaxSinks];
        usize sinkCount = 0;
        std::atomic<bool> consoleEnabled{true};

        std::mutex& SinkMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        void WriteConsole(const LogMessage& m)
        {
#if HX_PLATFORM_ANDROID
            static const int priorities[] = {ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_FATAL};
            __android_log_print(priorities[int(m.level)], m.channel->name, "%.*s", int(m.text.Size()), m.text.Data());
#else
            char line[1200];
            int n = std::snprintf(line, sizeof(line), "[%s] %s: %.*s\n", LogLevelName(m.level), m.channel->name, int(m.text.Size()), m.text.Data());
            std::FILE* stream = m.level >= LogLevel::Warning ? stderr : stdout;
            std::fwrite(line, 1, n > 0 ? Min<usize>(usize(n), sizeof(line) - 1) : 0, stream);
#if HX_PLATFORM_WINDOWS
            if (IsDebuggerPresent())
                OutputDebugStringA(line);
#endif
#endif
        }
    }

    const char* LogLevelName(LogLevel level)
    {
        static const char* names[] = {"trace", "debug", "info", "warning", "error", "fatal"};
        return names[int(level)];
    }

    bool AddLogSink(LogSink sink, void* user)
    {
        std::lock_guard lock(SinkMutex());
        if (sinkCount == kMaxSinks)
            return false;
        sinks[sinkCount++] = {sink, user};
        return true;
    }

    void RemoveLogSink(LogSink sink, void* user)
    {
        std::lock_guard lock(SinkMutex());
        for (usize i = 0; i < sinkCount; i++)
        {
            if (sinks[i].sink == sink && sinks[i].user == user)
            {
                sinks[i] = sinks[--sinkCount];
                return;
            }
        }
    }

    void SetConsoleLogging(bool enabled) { consoleEnabled.store(enabled, std::memory_order_relaxed); }

    void WriteLog(const LogChannel& channel, LogLevel level, const char* file, int line, StringView text)
    {
        LogMessage message{&channel, level, text, file, line};
        if (consoleEnabled.load(std::memory_order_relaxed))
            WriteConsole(message);
        std::lock_guard lock(SinkMutex());
        for (usize i = 0; i < sinkCount; i++)
            sinks[i].sink(message, sinks[i].user);
    }
}
