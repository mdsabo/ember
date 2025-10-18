#include <array>
#include <catch2/catch_test_macros.hpp>

#include "Log.h"

using namespace ember::util;

class TestLogger final : public Logger {
public:
    LogManifest manifest;
    std::string msg;

    virtual void vformat(const LogManifest& manifest, std::string_view fmt, std::format_args args) {
        this->manifest = manifest;
        msg = std::vformat(fmt, args);
    }
};

TEST_CASE("ember log macros write manifest, fmt and args to logger vformat function", "[Log]") {
    auto logger = std::make_shared<TestLogger>();
    set_logger(logger);

    info("test-log", "Hello, {}", "World"); auto line = __LINE__;

    REQUIRE(logger->msg == "Hello, World");
    REQUIRE(std::string(logger->manifest.func) == __FUNCTION__);
    REQUIRE(std::string(logger->manifest.file) == __FILE__);
    REQUIRE(logger->manifest.lineno == line);
    REQUIRE(logger->manifest.level == LogLevel::Info);
    REQUIRE(std::string("test-log") == logger->manifest.target);
}
