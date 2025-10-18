#pragma once

#include <format>
#include <memory>
#include <string>
#include <string_view>

#ifndef EMBER_STATIC_LOG_LEVEL
#define EMBER_STATIC_LOG_LEVEL 0
#endif

#define __ember_log_impl(level, target, fmt, ...) \
    ember::util::log({target, level, __FUNCTION__, __FILE__, __LINE__}, fmt, ## __VA_ARGS__)

#if EMBER_STATIC_LOG_LEVEL <= 3
#define error(fmt, ...) \__ember_log_impl(ember::util::LogLevel::Error, target, fmt, __VA_ARGS__)
#else
#define error(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL <= 2
#define warn(target, fmt, ...) __ember_log_impl(ember::util::LogLevel::Warn, target, fmt, __VA_ARGS__)
#else
#define warn(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL <= 1
#define info(target, fmt, ...) __ember_log_impl(ember::util::LogLevel::Info, target, fmt, __VA_ARGS__)
#else
#define info(fmt, ...)
#endif

#if EMBER_STATIC_LOG_LEVEL <= 0
#define trace(target, fmt, ...) __ember_log_impl(ember::util::LogLevel::Trace, target, fmt, __VA_ARGS__)
#else
#define trace(fmt, ...)
#endif

namespace ember::util {

    enum class LogLevel {
        Trace,
        Info,
        Warn,
        Error
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
        virtual void vformat(
            const LogManifest& manifest,
            std::string_view fmt,
            std::format_args args
        ) = 0;
    };

    void set_logger(std::shared_ptr<Logger> logger);
    void set_maximum_log_level(LogLevel level);
    LogLevel get_maximum_log_level();

    void __vlog(const LogManifest& manifest, std::string_view fmt, std::format_args args);

    template<typename... Args>
    inline void log(
        const LogManifest& manifest,
        const std::format_string<Args...> fmt,
        Args&&... args
    ) {
        __vlog(manifest, fmt.get(), std::make_format_args(args...));
    }
}