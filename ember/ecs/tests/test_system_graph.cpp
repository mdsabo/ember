#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "SystemGraph.h"

using namespace ember::ecs;

#define DEFINE_SYSTEM(name) \
struct name {                                   \
    static void init(World& world) { }          \
    static void run(World& world, float dt) { } \
}

DEFINE_SYSTEM(SystemA);
DEFINE_SYSTEM(SystemB);
DEFINE_SYSTEM(SystemC);
DEFINE_SYSTEM(SystemD);
DEFINE_SYSTEM(SystemE);
DEFINE_SYSTEM(SystemF);

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

    REQUIRE(systems.size() == 2); // 2 phases
    REQUIRE(systems.at(0).contains(SystemA::run));
    REQUIRE(systems.at(1).contains(SystemB::run));
}

TEST_CASE("SystemGraph 3", "[SystemGraph]") {
    SystemGraphBuilder builder;

    builder.add_system<SystemA>();
    builder.add_system<SystemB>();
    builder.add_system<SystemC>();
    builder.add_system<SystemD>();
    builder.add_system<SystemE>();
    builder.add_system<SystemF>();

    builder.order_systems<SystemA, SystemB>();
    builder.order_systems<SystemB, SystemC>();

    builder.order_systems<SystemA, SystemD>();

    builder.order_systems<SystemE, SystemC>();

    builder.order_systems<SystemA, SystemF>();
    builder.order_systems<SystemC, SystemF>();
    builder.order_systems<SystemE, SystemF>();

    auto systems = builder.build();

    REQUIRE(systems.size() == 4); // 4 phases

    // Phase 0
    REQUIRE(systems.at(0).size() == 2);
    REQUIRE(systems.at(0).contains(SystemA::run));
    REQUIRE(systems.at(0).contains(SystemE::run));

    // Phase 1
    REQUIRE(systems.at(1).size() == 2);
    REQUIRE(systems.at(1).contains(SystemB::run));
    REQUIRE(systems.at(1).contains(SystemD::run));

    // Phase 2
    REQUIRE(systems.at(2).size() == 1);
    REQUIRE(systems.at(2).contains(SystemC::run));

    // Phase 3
    REQUIRE(systems.at(3).size() == 1);
    REQUIRE(systems.at(3).contains(SystemF::run));
}