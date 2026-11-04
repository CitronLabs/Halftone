module;
#include <format>
#include <string>
#include <string_view>
#include <utility>

export module com.citronlabs.Halftone.Core:Log;

import :Types;

export namespace Halftone::Core
{
    enum class LogLevel : u8 { Trace, Info, Warn, Error, Fatal };

    // Thin, swappable logging front-end. The sink is a plain function so the
    // Editor can later route messages into an in-app console panel.
    class Log
    {
    public:
        Log() = delete;

        using Sink = void(*)(LogLevel, StringView channel, StringView message);

        static void SetSink(Sink sink);
        static void SetMinimumLevel(LogLevel level);
        static void Write(LogLevel level, StringView channel, StringView message);

        template<typename... Args>
        static void Trace(StringView channel, std::format_string<Args...> fmt, Args&&... args)
        { Write(LogLevel::Trace, channel, std::format(fmt, std::forward<Args>(args)...)); }

        template<typename... Args>
        static void Info(StringView channel, std::format_string<Args...> fmt, Args&&... args)
        { Write(LogLevel::Info, channel, std::format(fmt, std::forward<Args>(args)...)); }

        template<typename... Args>
        static void Warn(StringView channel, std::format_string<Args...> fmt, Args&&... args)
        { Write(LogLevel::Warn, channel, std::format(fmt, std::forward<Args>(args)...)); }

        template<typename... Args>
        static void Error(StringView channel, std::format_string<Args...> fmt, Args&&... args)
        { Write(LogLevel::Error, channel, std::format(fmt, std::forward<Args>(args)...)); }

        template<typename... Args>
        static void Fatal(StringView channel, std::format_string<Args...> fmt, Args&&... args)
        { Write(LogLevel::Fatal, channel, std::format(fmt, std::forward<Args>(args)...)); }
    };
}
