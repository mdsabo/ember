#include <array>
#include <catch2/catch_test_macros.hpp>

#include "ArgParser.h"

using namespace ember::util;

TEST_CASE("ArgParser::is_set() returns true if requested arg is in argv", "[ArgParser]") {
    constexpr std::array ARGS = { "ProgramName", "-a", "--all", "-b=3", "a", "b", "abcd" };

    ArgParser parser(static_cast<int>(ARGS.size()), ARGS.data());

    REQUIRE(parser.is_set("-a"));
    REQUIRE(parser.is_set("--all"));
    REQUIRE(parser.is_set("-b"));

    REQUIRE_FALSE(parser.is_set("-x"));
    REQUIRE_FALSE(parser.is_set("--no"));
    REQUIRE_FALSE(parser.is_set("a"));
}
