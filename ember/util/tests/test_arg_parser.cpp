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
}

TEST_CASE("ArgParser::arg_value() returns arg value if arg uses assignment", "[ArgParser]") {
    constexpr std::array ARGS = { "ProgramName", "-b=3" };
    ArgParser parser(static_cast<int>(ARGS.size()), ARGS.data());

    REQUIRE(parser.arg_value("-b") == "3");
}

TEST_CASE("ArgParser::arg_value() returns arg value if value is passed as next arg", "[ArgParser]") {
    constexpr std::array ARGS = { "ProgramName", "-b", "3" };
    ArgParser parser(static_cast<int>(ARGS.size()), ARGS.data());

    REQUIRE(parser.arg_value("-b") == "3");
}

TEST_CASE("ArgParser::arg_value() throws out_of_range if no value is passed", "[ArgParser]") {
    constexpr std::array ARGS = { "ProgramName", "-b" };
    ArgParser parser(static_cast<int>(ARGS.size()), ARGS.data());

    REQUIRE_THROWS_AS(parser.arg_value("-b"), std::out_of_range);
}
