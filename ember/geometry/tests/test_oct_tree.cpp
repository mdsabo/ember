#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <glm/gtc/random.hpp>
#include <vector>

#include "Intersect.h"
#include "OctTree.h"

using namespace ember::geometry;

constexpr auto WORLD_MIN = glm::vec3(-10.0, -10.0, -10.0);
constexpr auto WORLD_MAX = glm::vec3(10.0, 10.0, 10.0);
constexpr auto WORLD_AABB = AABB{
    .center = glm::vec3(),
    .half_size = WORLD_MAX
};

std::vector<OctTree<int>::Element> get_test_objects(size_t count) {
    std::vector<OctTree<int>::Element> objects(count);
    for (auto i = 0; i < count; i++) {
        objects[i].data = i;
        objects[i].aabb.center = glm::linearRand(WORLD_MIN, WORLD_MAX);
        objects[i].aabb.half_size = glm::linearRand(glm::vec3(), WORLD_MAX)/10.0f;
    }
    return objects;
}

TEST_CASE("QuadTree::query(aabb) returns all entries that intersect an aabb", "[QuadTree]") {
    constexpr auto OBJ_COUNT = 1000;
    auto qt = OctTree<int>(WORLD_AABB);
    const auto objects = get_test_objects(OBJ_COUNT);
    for (const auto& obj : objects) qt.insert(obj);

    constexpr auto TEST_BOX = AABB{
        .center = glm::vec3(6.0, 4.0, -5.0),
        .half_size = glm::vec3(3.0, 3.0, 1.0)
    };

    const auto intersections = qt.query(TEST_BOX);

    for (auto i = 0; i < OBJ_COUNT; i++) {
        if (intersect(TEST_BOX, objects[i].aabb)) {
            const auto iter = std::find(intersections.begin(), intersections.end(), i);
            REQUIRE(iter != intersections.end());
        }
    }
}

TEST_CASE("OctTree::insert benchmarks", "[OctTree]") {
    BENCHMARK_ADVANCED("1K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 1000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };

    BENCHMARK_ADVANCED("10K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 10000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };

    BENCHMARK_ADVANCED("100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };
}

TEST_CASE("OctTree::query benchmarks", "[OctTree]") {
    BENCHMARK_ADVANCED("1K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 1000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&qt, &TEST_BOX]() { return qt.query(TEST_BOX); });
    };

    BENCHMARK_ADVANCED("Brute force 1K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 1000;
        const auto objects = get_test_objects(OBJ_COUNT);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&TEST_BOX, &objects]() {
            std::vector<int> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };

    BENCHMARK_ADVANCED("10K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 10000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&qt, &TEST_BOX]() { return qt.query(TEST_BOX); });
    };

    BENCHMARK_ADVANCED("Brute force 10K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 10000;
        const auto objects = get_test_objects(OBJ_COUNT);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&TEST_BOX, &objects]() {
            std::vector<int> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };

    BENCHMARK_ADVANCED("100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        auto qt = OctTree<int>(WORLD_AABB);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&qt, &TEST_BOX]() { return qt.query(TEST_BOX); });
    };

    BENCHMARK_ADVANCED("Brute force 100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        const auto objects = get_test_objects(OBJ_COUNT);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&TEST_BOX, &objects]() {
            std::vector<int> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };
}