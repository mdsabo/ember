#include "Log.h"

#include <mutex>

namespace ember::util {

    struct SharedLogger {
        LogLevel max_log_level;
        std::shared_ptr<Logger> logger;
        std::mutex mutex;
    };
    static SharedLogger s_shared_logger;

    void set_logger(std::shared_ptr<Logger> logger) {
        std::lock_guard lock(s_shared_logger.mutex);
        s_shared_logger.logger = logger;
    }

    void set_maximum_log_level(LogLevel level) {
        std::lock_guard lock(s_shared_logger.mutex);
        s_shared_logger.max_log_level = level;
    }

    void __vlog(const LogManifest& manifest, std::string_view fmt, std::format_args args) {
        std::lock_guard lock(s_shared_logger.mutex);

        auto& logger = s_shared_logger.logger;
        if (logger != nullptr) logger->vformat(manifest, fmt, args);
    }

}