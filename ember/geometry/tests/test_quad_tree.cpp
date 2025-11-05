#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <glm/gtc/random.hpp>
#include <vector>

#include "Intersect.h"
#include "QuadTree.h"

using namespace ember::geometry;

constexpr auto WORLD_SIZE = 256.0f;
constexpr auto WORLD_MIN = glm::vec3(-WORLD_SIZE, -WORLD_SIZE, -WORLD_SIZE);
constexpr auto WORLD_MAX = glm::vec3(WORLD_SIZE, WORLD_SIZE, WORLD_SIZE);
constexpr auto WORLD_AABB = AABB{
    .center = glm::vec3(),
    .half_size = WORLD_MAX
};

std::vector<QuadTree<uint64_t>::Element> get_test_objects(size_t count) {
    std::vector<QuadTree<uint64_t>::Element> objects(count);
    for (auto i = 0; i < count; i++) {
        objects[i].data = i;
        objects[i].aabb.center = glm::linearRand(WORLD_MIN+5.0f, WORLD_MAX-5.0f);
        objects[i].aabb.half_size = glm::linearRand(glm::vec3(0.5, 0.5, 0.5), glm::vec3(5.0, 5.0, 5.0));
    }
    return objects;
}

TEST_CASE("QuadTree::query(aabb) returns all entries that intersect an aabb", "[QuadTree]") {
    constexpr auto OBJ_COUNT = 2500;
    auto qt = QuadTree<uint64_t>(WORLD_AABB);
    const auto objects = get_test_objects(OBJ_COUNT);
    for (const auto& obj : objects) qt.insert(obj);

    constexpr auto TEST_BOX = AABB{
        .center = glm::vec3(26.0, 0.0, -25.0),
        .half_size = glm::vec3(23.0, WORLD_SIZE, 21.0)
    };

    auto intersections = qt.query(TEST_BOX);
    std::sort(intersections.begin(), intersections.end());

    std::vector<uint64_t> brute_force_intersections;
    for (auto i = 0; i < OBJ_COUNT; i++) {
        if (intersect(TEST_BOX, objects[i].aabb)) {
            brute_force_intersections.push_back(objects[i].data);
        }
    }
    std::sort(brute_force_intersections.begin(), brute_force_intersections.end());

    REQUIRE(brute_force_intersections.size() >= 10);
    REQUIRE(intersections == brute_force_intersections);
}

TEST_CASE("QuadTree::insert benchmarks", "[QuadTree]") {
    BENCHMARK_ADVANCED("1K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 1000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
        const auto objects = get_test_objects(OBJ_COUNT);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };

    BENCHMARK_ADVANCED("10K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 10000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
        const auto objects = get_test_objects(OBJ_COUNT);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };

    BENCHMARK_ADVANCED("100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
        const auto objects = get_test_objects(OBJ_COUNT);

        meter.measure([&qt, &objects]() {
            for (const auto& obj : objects) qt.insert(obj);
            return &qt;
        });
    };
}

TEST_CASE("QuadTree::query benchmarks", "[QuadTree]") {
    BENCHMARK_ADVANCED("1K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 1000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
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
            std::vector<uint64_t> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };

    BENCHMARK_ADVANCED("10K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 10000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
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
            std::vector<uint64_t> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };

    BENCHMARK_ADVANCED("100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        auto qt = QuadTree<uint64_t>(WORLD_AABB, OBJ_COUNT);
        const auto objects = get_test_objects(OBJ_COUNT);
        for (const auto& obj : objects) qt.insert(obj);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&qt, &TEST_BOX]() {
            auto i = qt.query(TEST_BOX);
        });
    };

    BENCHMARK_ADVANCED("Brute force 100K objects")(Catch::Benchmark::Chronometer meter) {
        constexpr auto OBJ_COUNT = 100000;
        const auto objects = get_test_objects(OBJ_COUNT);

        constexpr auto TEST_BOX = AABB{
            .center = glm::vec3(6.0, 4.0, -5.0),
            .half_size = glm::vec3(3.0, 3.0, 1.0)
        };

        meter.measure([&TEST_BOX, &objects]() {
            std::vector<uint64_t> intersections;
            for (const auto& obj : objects) {
                if (intersect(obj.aabb, TEST_BOX)) intersections.push_back(obj.data);
            }
            return intersections;
        });
    };
}