#include <catch2/catch_test_macros.hpp>

#include "Storage.h"
#include "World.h"

using namespace ember::ecs;

struct TestComponent {
    using Storage = DenseVectorStorage<TestComponent>;
    int value;
};
static_assert(Component<TestComponent>);

struct TestComponent2 {
    using Storage = DenseVectorStorage<TestComponent2>;
    int value;
};
static_assert(Component<TestComponent2>);

TEST_CASE("World::create_entity returns new or recycled entity", "[World]") {
    World world;

    auto e0 = world.create_entity();
    auto e1 = world.create_entity();
    REQUIRE(e0 != e1);

    world.destroy_entity(e0);
    auto e0_1 = world.create_entity();
    REQUIRE(e0_1 == (e0.generation++));
}

TEST_CASE("World::query", "[World]") {
    World world;
    world.add_component<TestComponent>();
    world.add_component<TestComponent2>();

    auto& t0 = world.write_component<TestComponent>();
    t0.insert(0, {0});
    t0.insert(1, {1});

    auto& t1 = world.write_component<TestComponent2>();
    t1.insert(0, {0});

    const auto query = world.query<TestComponent, TestComponent2>();
    REQUIRE(query.contains(0));
    REQUIRE_FALSE(query.contains(1));
}