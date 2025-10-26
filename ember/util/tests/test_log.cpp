#include <array>
#include <catch2/catch_test_macros.hpp>

#include "Log.h"

using namespace ember::util;

class TestLogger final : public Logger {
public:
    LogManifest manifest;
    std::string msg;

    virtual void write(const LogManifest& manifest, const std::string& msg) override {
        this->manifest = manifest;
        this->msg = msg;
    }
};

TEST_CASE("ember log macros write manifest, fmt and args to logger vformat function", "[Log]") {
    auto logger = std::make_unique<TestLogger>();

    log(
        logger.get(),
        { "test-log", LogLevel::Info, "function", "file", 420},
        "Hello, {}", "World"
    );

    REQUIRE(std::string("test-log") == logger->manifest.target);
    REQUIRE(logger->manifest.level == LogLevel::Info);
    REQUIRE(std::string(logger->manifest.func) == "function");
    REQUIRE(std::string(logger->manifest.file) == "file");
    REQUIRE(logger->manifest.lineno == 420);
    REQUIRE(logger->msg == "Hello, World");
}
