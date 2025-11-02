#include <catch2/catch_test_macros.hpp>

#include "Intersect.h"

using namespace ember::geometry;

TEST_CASE("intersect returns Some(...) iff two spheres intersect", "[intersect]") {
    auto s1 = Sphere{
        .center = glm::vec3(),
        .radius = 1.0f
    };
    auto s2 = Sphere{
        .center = glm::vec3(1.0, 1.0, 1.0),
        .radius = 1.0f
    };
    auto s3 = Sphere{
        .center = glm::vec3(2.0, 2.0, 2.0),
        .radius = 1.0f
    };

    REQUIRE(intersect(s1, s2));
    REQUIRE(intersect(s2, s3));
    REQUIRE_FALSE(intersect(s1, s3));
}

TEST_CASE("intersect returns Some(...) if outer sphere encloses inner one", "[intersect]") {
    auto s1 = Sphere{
        .center = glm::vec3(),
        .radius = 1.0f
    };
    auto s2 = Sphere{
        .center = glm::vec3(1.0, 1.0, 1.0),
        .radius = 10.0f
    };

    REQUIRE(intersect(s1, s2));
}

// TEST_CASE("intersect returns true if sphere center is within AABB", "[intersect]") {
//     auto s = Sphere{
//         .center = glm::vec3(),
//         .radius = 1.0f
//     };
//     auto a = AABB {
//         .min = glm::vec3(),
//         .max = glm::vec3(0.75, 0.75, 0.75)
//     };

//     REQUIRE(intersect(s, a));
// }

// TEST_CASE("intersect returns true if sphere center is less than radius away from AABB", "[intersect]") {
//     auto s = Sphere{
//         .center = glm::vec3(1.0, 1.0, 1.0),
//         .radius = 1.0f
//     };
//     auto a = AABB {
//         .min = glm::vec3(),
//         .max = glm::vec3(0.75, 0.75, 0.75)
//     };

//     REQUIRE(intersect(s, a));
// }

// TEST_CASE("intersect returns false if sphere center is more than radius away from AABB", "[intersect]") {
//     auto s = Sphere{
//         .center = glm::vec3(2.0, 2.0, 2.0),
//         .radius = 1.0f
//     };
//     auto a = AABB {
//         .min = glm::vec3(),
//         .max = glm::vec3(0.75, 0.75, 0.75)
//     };

//     REQUIRE_FALSE(intersect(s, a));
// }