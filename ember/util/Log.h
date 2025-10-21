#pragma once

#include <format>
#include <memory>
#include <string>
#include <string_view>

#ifndef EMBER_STATIC_LOG_LEVEL
#define EMBER_STATIC_LOG_LEVEL 100
#endif

#define __ember_log_impl(level, target, fmt, ...) \
    ember::util::log(ember::util::get_global_logger(), {target, level, __FUNCTION__, __FILE__, __LINE__}, fmt, ## __VA_ARGS__)

#if EMBER_STATIC_LOG_LEVEL >= 0
#define error(target, fmt, ...) \
    __ember_log_impl(ember::util::LogLevel::Error, target, fmt, ## __VA_ARGS__)
#else
#define errorl(fmt, ...)
#define error(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL >= 1
#define warn(target, fmt, ...) \
    __ember_log_impl(ember::util::LogLevel::Warn, target, fmt, ## __VA_ARGS__)
#else
#define warnl(fmt, ...)
#define warn(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL >= 2
#define info(target, fmt, ...) \
    __ember_log_impl(ember::util::LogLevel::Info, target, fmt, ## __VA_ARGS__)
#else
#define infol(fmt, ...)
#define info(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL >= 3
#define trace(target, fmt, ...) \
    __ember_log_impl(ember::util::LogLevel::Trace, target, fmt, ## __VA_ARGS__)
#else
#define tracel(fmt, ...)
#define trace(fmt, ...)
#endif

namespace ember::util {

    enum class LogLevel {
        Error,
        Warn,
        Info,
        Trace
    };

    struct LogManifest {
        std::string_view target;
        LogLevel level;
        const char* func;
        const char* file;
        int lineno;
    };

    class Logger {
    public:
        virtual ~Logger() = default;
        virtual void write(
            const LogManifest& manifest, const std::string& msg) = 0;
    };

    void set_global_logger(std::unique_ptr<Logger> logger);
    Logger* get_global_logger();

    void set_maximum_log_level(LogLevel level);
    LogLevel get_maximum_log_level();

    void __vlog(
        Logger* logger,
        const LogManifest& manifest,
        std::string_view fmt,
        std::format_args args
    );

    template<typename... Args>
    inline void log(
        Logger* logger,
        const LogManifest& manifest,
        const std::format_string<Args...> fmt,
        Args&&... args
    ) {
        __vlog(logger, manifest, fmt.get(), std::make_format_args(args...));
    }

    class PrintfLogger : public Logger {
    public:
        virtual void write(const LogManifest& manifest, const std::string& msg) override;
    };
}

constexpr std::string to_string(ember::util::LogLevel level) {
    switch(level) {
    case ember::util::LogLevel::Error: return "Error";
    case ember::util::LogLevel::Warn: return "Warn";
    case ember::util::LogLevel::Info: return "Info";
    case ember::util::LogLevel::Trace: return "Trace";
    }
}