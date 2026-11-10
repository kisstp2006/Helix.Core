#pragma once

#include <helix/core/strings/format.h>

namespace helix
{
    enum class LogLevel : u8
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    struct LogChannel
    {
        const char* name;
        LogLevel minimum = LogLevel::Info;
    };

    struct LogMessage
    {
        const LogChannel* channel;
        LogLevel level;
        StringView text;
        const char* file;
        int line;
    };

    using LogSink = void (*)(const LogMessage& message, void* user);

    bool AddLogSink(LogSink sink, void* user);
    void RemoveLogSink(LogSink sink, void* user);
    void SetConsoleLogging(bool enabled);
    const char* LogLevelName(LogLevel level);
    void WriteLog(const LogChannel& channel, LogLevel level, const char* file, int line, StringView text);

    extern LogChannel CoreLog;

    template <typename... Args>
    void Log(const LogChannel& channel, LogLevel level, const char* file, int line, StringView format, const Args&... args)
    {
        if (level < channel.minimum)
            return;
        char buffer[1024];
        usize size = FormatToBuffer(buffer, sizeof(buffer), format, args...);
        if (size < sizeof(buffer))
        {
            WriteLog(channel, level, file, line, StringView(buffer, size));
            return;
        }
        String text = Format(format, args...);
        WriteLog(channel, level, file, line, text.View());
    }
}

#define HX_LOG(channel, level, ...) ::helix::Log(channel, ::helix::LogLevel::level, __FILE__, __LINE__, __VA_ARGS__)
#define HX_LOG_TRACE(channel, ...) HX_LOG(channel, Trace, __VA_ARGS__)
#define HX_LOG_DEBUG(channel, ...) HX_LOG(channel, Debug, __VA_ARGS__)
#define HX_LOG_INFO(channel, ...) HX_LOG(channel, Info, __VA_ARGS__)
#define HX_LOG_WARNING(channel, ...) HX_LOG(channel, Warning, __VA_ARGS__)
#define HX_LOG_ERROR(channel, ...) HX_LOG(channel, Error, __VA_ARGS__)
