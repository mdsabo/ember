#include <catch2/catch_template_test_macros.hpp>

#include "Storage.h"

using namespace ember::ecs;

TEMPLATE_TEST_CASE("Storage::contains() returns true iff the entity is in the storage", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;
    storage.insert(0, 5);

    REQUIRE(storage.contains(0));
    REQUIRE_FALSE(storage.contains(1));
}

TEMPLATE_TEST_CASE("Storage::entities() returns entity set of the storage", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;
    storage.insert(0, 5);

    const auto& entities = storage.entities();
    REQUIRE(entities.contains(0));
    REQUIRE_FALSE(entities.contains(1));
}

TEMPLATE_TEST_CASE("Storage::operator[] returns component reference", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;

    storage.insert(0, 5);
    storage[0] = 6;
    REQUIRE(storage[0] == 6);

    const TestType& const_storage = storage;
    REQUIRE(storage[0] == 6);
}

TEMPLATE_TEST_CASE("Storage::at() returns component reference if storage holds entity", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;

    storage.insert(0, 5);
    storage.at(0) = 6;
    REQUIRE(storage.at(0) == 6);

    const TestType& const_storage = storage;
    REQUIRE(storage.at(0) == 6);

    storage.remove(0);
    REQUIRE_THROWS_AS(storage.at(0), std::out_of_range);
}

TEMPLATE_TEST_CASE("Storage::insert() overwites existing components for the same entity", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;
    storage.insert(0, 0);
    storage.insert(0, 1);
    REQUIRE(storage[0] == 1);
}

TEMPLATE_TEST_CASE("Storage::remove() throws out_of_range if entity is not in storage", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>, MapStorage<int>) {
    TestType storage;
    storage.insert(0, 0);
    REQUIRE_THROWS_AS(storage.remove(1), std::out_of_range);
}

TEMPLATE_TEST_CASE("Storage::begin/end() allow iteration over the components", "[Storage]", VectorStorage<int>, DenseVectorStorage<int>) {
    TestType storage;
    for (auto i = 0; i < 10; i++) {
        storage.insert(i, i);
    }

    auto i = 0;
    for (auto iter = storage.begin(); iter != storage.end(); iter++, i++) {
        *iter = 2*i;
        REQUIRE(*iter == 2*i);
    }

    const auto& const_storage = storage;
    i = 0;
    for (auto iter = const_storage.begin(); iter != const_storage.end(); iter++, i++) {
        REQUIRE(*iter == 2*i);
    }
}

TEST_CASE("MapStorage::begin/end() allow iteration over the components", "[Storage]") {
    MapStorage<int> storage;
    for (auto i = 0; i < 10; i++) {
        storage.insert(i, i);
    }

    auto i = 0;
    for (auto iter = storage.begin(); iter != storage.end(); iter++, i++) {
        iter->second = 2*i;
        REQUIRE(iter->second == 2*i);
    }

    const auto& const_storage = storage;
    i = 0;
    for (auto iter = const_storage.begin(); iter != const_storage.end(); iter++, i++) {
        REQUIRE(iter->second == 2*i);
    }
}
