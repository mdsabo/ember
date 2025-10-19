#include "Log.h"

#include <atomic>
#include <filesystem>
#include <iostream>

namespace ember::util {

    struct GlobalLoggerState {
        std::atomic<LogLevel> max_log_level = LogLevel::Trace;
        std::unique_ptr<Logger> logger = nullptr;
    };
    static GlobalLoggerState s_global_logger_state;

    void set_global_logger(std::unique_ptr<Logger> logger) {
        s_global_logger_state.logger = std::move(logger);
    }

    Logger* get_global_logger() {
        return s_global_logger_state.logger.get();
    }

    void set_maximum_log_level(LogLevel level) {
        s_global_logger_state.max_log_level.store(level);
    }

    LogLevel get_maximum_log_level() {
        return s_global_logger_state.max_log_level.load();
    }

    void __vlog(
        Logger* logger,
        const LogManifest& manifest,
        std::string_view fmt,
        std::format_args args
    ) {
        if ((logger != nullptr) && (manifest.level <= get_maximum_log_level())) {
            logger->write(manifest, std::vformat(fmt, args));
        }
    }


    void PrintfLogger::write(const LogManifest& manifest, const std::string& msg) {
        auto filename = std::filesystem::path(manifest.file).filename();
        std::cout << std::format(
            "[{}][{}][{}::{}] ",
            to_string(manifest.level),
            manifest.target,
            filename.string(),
            manifest.lineno
        );
        std::cout << msg <<std::endl;
    }
}