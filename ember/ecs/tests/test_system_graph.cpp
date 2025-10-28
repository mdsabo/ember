#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "SystemGraph.h"

using namespace ember::ecs;

struct SystemA {
    static void init(World& world) { }
    static void run(World& world, float dt) { }
};
static_assert(System<SystemA>);

struct SystemB {
    static void init(World& world) { }
    static void run(World& world, float dt) { }
};
static_assert(System<SystemB>);

TEST_CASE("SystemGraph 1", "[SystemGraph]") {
    SystemGraphBuilder builder;

    builder.add_system<SystemA>();

    auto systems = builder.build();

    REQUIRE(systems.size() == 1); // 1 phase
    REQUIRE(systems.at(0).contains(SystemA::run));
}

TEST_CASE("SystemGraph 2", "[SystemGraph]") {
    SystemGraphBuilder builder;

    builder.add_system<SystemA>();
    builder.add_system<SystemB>();

    builder.order_systems<SystemA, SystemB>();

    auto systems = builder.build();

    REQUIRE(systems.size() == 1); // 2 phase2
    REQUIRE(systems.at(0).contains(SystemA::run));
    REQUIRE(systems.at(1).contains(SystemB::run));
}