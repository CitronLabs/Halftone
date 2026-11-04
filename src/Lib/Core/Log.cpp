module;
#include <cstdio>
#include <string_view>

module com.citronlabs.Halftone.Core;

using namespace Halftone::Core;

namespace
{
    const char* LevelName(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace: return "trace";
            case LogLevel::Info:  return "info ";
            case LogLevel::Warn:  return "warn ";
            case LogLevel::Error: return "error";
            case LogLevel::Fatal: return "FATAL";
        }
        return "?";
    }

    void DefaultSink(LogLevel level, StringView channel, StringView message)
    {
        FILE* stream = level >= LogLevel::Warn ? stderr : stdout;
        std::fprintf(stream, "[%s] [%.*s] %.*s\n",
            LevelName(level),
            int(channel.size()), channel.data(),
            int(message.size()), message.data());
        std::fflush(stream);
    }

    Log::Sink   g_Sink     = &DefaultSink;
#ifdef HALFTONE_DEBUG
    LogLevel    g_MinLevel = LogLevel::Trace;
#else
    LogLevel    g_MinLevel = LogLevel::Info;
#endif
}

void Log::SetSink(Sink sink)              { g_Sink = sink ? sink : &DefaultSink; }
void Log::SetMinimumLevel(LogLevel level) { g_MinLevel = level; }

void Log::Write(LogLevel level, StringView channel, StringView message)
{
    if (level < g_MinLevel) return;
    g_Sink(level, channel, message);
}
