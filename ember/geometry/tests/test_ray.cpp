#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "Ray.h"

using namespace ember::geometry;

TEST_CASE("Ray::origin returns the ray's origin", "[Ray]") {
    auto ray = Ray {
        { 0.0, 0.0, 0.0 },
        { 1.0, 0.0, 0.0 },
        1.0
    };
    REQUIRE(ray.origin == Eigen::Vector3f(0.0, 0.0, 0.0));
}
