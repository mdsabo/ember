#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "EntitySet.h"
#include "Storage.h"

using namespace ember::ecs;

TEST_CASE("EntitySet() creates new set with optional size", "[EntitySet]") {
    auto zero_size = EntitySet();
    REQUIRE(zero_size.size() == 0);

    auto set = EntitySet(1024);
    REQUIRE(set.size() == 1024);
}

TEST_CASE("EntitySet(const EntitySet&) copies the set", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    auto set_b = EntitySet(set_a);
    REQUIRE(set_b[1]);
    REQUIRE(set_b[23]);
    REQUIRE(set_b[456]);

    // operator= implemented by compiler
    auto set_c = set_b;
    REQUIRE(set_c[1]);
    REQUIRE(set_c[23]);
    REQUIRE(set_c[456]);
}

TEST_CASE("EntitySet(EntitySet&&) moves the set", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    auto set_b = EntitySet(std::move(set_a));
    REQUIRE(set_b[1]);
    REQUIRE(set_b[23]);
    REQUIRE(set_b[456]);

    // operator= implemented by compiler
    auto set_c = std::move(set_b);
    REQUIRE(set_c[1]);
    REQUIRE(set_c[23]);
    REQUIRE(set_c[456]);
}

TEST_CASE("EntitySet() has no entities", "[EntitySet]") {
    auto set = EntitySet(4);
    for (auto i = 0; i < 4; i++) {
        REQUIRE_FALSE(set[i]);
    }
}

TEST_CASE("EntitySet::contains() returns true iff entity is in set", "[EntitySet]") {
    auto set = EntitySet(1024);

    set.insert(1);
    set.insert(23);
    set.insert(456);

    REQUIRE_FALSE(set.contains(0));
    REQUIRE(set.contains(1));
    REQUIRE(set.contains(23));
    REQUIRE(set.contains(456));
    REQUIRE_FALSE(set.contains(1025));
}

TEST_CASE("EntitySet::remove() removes the entity from the set", "[EntitySet]") {
    auto set = EntitySet(1024);
    set.insert(1);
    set.insert(23);
    set.insert(456);

    set.remove(1);
    set.remove(23);
    set.remove(456);

    REQUIRE_FALSE(set.contains(1));
    REQUIRE_FALSE(set.contains(23));
    REQUIRE_FALSE(set.contains(456));
}


TEST_CASE("EntitySet::resize() sets the set size", "[EntitySet]") {
    auto set = EntitySet(1024);

    set.resize(56);
    REQUIRE(set.size() == 56);

    set.resize(1000);
    REQUIRE(set.size() == 1000);
}

TEST_CASE("EntitySet::resize() intializes entities to false", "[EntitySet]") {
    auto set = EntitySet(64);
    set.insert(60);

    set.resize(56);
    set.resize(128);

    REQUIRE_FALSE(set[60]);
    REQUIRE_FALSE(set[100]);
}

TEST_CASE("EntitySet::begin/end allow iterator over entity set", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    std::vector<Entity> entities;
    for (const auto e : set_a) {
        entities.push_back(e);
    }

    REQUIRE(entities.size() == 3);
    REQUIRE(entities.at(0) == 1);
    REQUIRE(entities.at(1) == 23);
    REQUIRE(entities.at(2) == 456);
}

TEST_CASE("EntitySet::as_vec returns contains entities as vector", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    std::vector<Entity> entities = set_a.as_vec();

    REQUIRE(entities.size() == 3);
    REQUIRE(entities.at(0) == 1);
    REQUIRE(entities.at(1) == 23);
    REQUIRE(entities.at(2) == 456);
}

TEST_CASE("EntitySet::operator&= computes intersection of two sets", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    auto set_b = EntitySet(1024);
    set_b.insert(1);
    set_b.insert(Entity(1, 23)); // different generation so no match

    set_a &= set_b;

    REQUIRE(set_a[1]);
    REQUIRE_FALSE(set_a[23]);
    REQUIRE_FALSE(set_a[456]);
}

TEST_CASE("EntitySet::operator& computes intersection of two sets", "[EntitySet]") {
    auto set_a = EntitySet(1024);
    set_a.insert(1);
    set_a.insert(23);
    set_a.insert(456);

    auto set_b = EntitySet(1024);
    set_b.insert(1);
    set_b.insert(Entity(1, 23)); // different generation so no match

    auto res = set_a & set_b;

    REQUIRE(res[1]);
    REQUIRE_FALSE(res[23]);
    REQUIRE_FALSE(res[456]);
}

TEST_CASE("EntitySet benchmarks", "[EntitySet]") {
    BENCHMARK_ADVANCED("Dense EntitySet intersect")(Catch::Benchmark::Chronometer meter) {
        auto set_a = EntitySet(50000);
        auto set_b = EntitySet(50000);
        for (auto i = 0; i < 50000; i++) {
            set_a.insert(i);
            set_b.insert(i);
        }
        meter.measure([set_a, set_b]() { return set_a & set_b; });
    };

    BENCHMARK_ADVANCED("Sparse EntitySet intersect")(Catch::Benchmark::Chronometer meter) {
        auto set_a = EntitySet(50000);
        auto set_b = EntitySet(50000);
        set_a.insert(5);
        set_b.insert(49995);
        meter.measure([set_a, set_b]() { return set_a & set_b; });
    };
}
