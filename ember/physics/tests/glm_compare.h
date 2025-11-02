#pragma once

#include <catch2/matchers/catch_matchers_floating_point.hpp>

#define REQUIRE_VEC_REL(vec, expected, rel)            \
    REQUIRE_THAT(vec.x, Catch::Matchers::WithinRel(expected.x, rel)); \
    REQUIRE_THAT(vec.y, Catch::Matchers::WithinRel(expected.y, rel)); \
    REQUIRE_THAT(vec.z, Catch::Matchers::WithinRel(expected.z, rel))

#define REQUIRE_VEC_ABS(vec, expected, abs)            \
    REQUIRE_THAT(vec.x, Catch::Matchers::WithinAbs(expected.x, abs)); \
    REQUIRE_THAT(vec.y, Catch::Matchers::WithinAbs(expected.y, abs)); \
    REQUIRE_THAT(vec.z, Catch::Matchers::WithinAbs(expected.z, abs))

#define REQUIRE_MAT3_REL(mat, expected, rel)   \
    REQUIRE_VEC_REL(mat[0], expected[0], rel); \
    REQUIRE_VEC_REL(mat[1], expected[1], rel); \
    REQUIRE_VEC_REL(mat[2], expected[2], rel)

#define REQUIRE_MAT3_ABS(mat, expected, abs)   \
    REQUIRE_VEC_REL(mat[0], expected[0], abs); \
    REQUIRE_VEC_REL(mat[1], expected[1], abs); \
    REQUIRE_VEC_REL(mat[2], expected[2], abs)

#define REQUIRE_MAT4_REL(mat, expected, rel)   \
    REQUIRE_VEC_REL(mat[0], expected[0], rel); \
    REQUIRE_VEC_REL(mat[1], expected[1], rel); \
    REQUIRE_VEC_REL(mat[2], expected[2], rel); \
    REQUIRE_VEC_REL(mat[3], expected[3], rel)

#define REQUIRE_MAT4_ABS(mat, expected, abs)   \
    REQUIRE_VEC_ABS(mat[0], expected[0], abs); \
    REQUIRE_VEC_ABS(mat[1], expected[1], abs); \
    REQUIRE_VEC_ABS(mat[2], expected[2], abs); \
    REQUIRE_VEC_ABS(mat[3], expected[3], abs)