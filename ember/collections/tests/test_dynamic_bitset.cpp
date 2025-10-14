#include <catch2/catch_test_macros.hpp>

#include "DynamicBitset.h"

using namespace ember::collections;

TEST_CASE("DynamicBitset(const DynamicBitset&) copies the bitset", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    auto bitset_b = DynamicBitset(bitset);
    REQUIRE(bitset_b[1]);
    REQUIRE(bitset_b[23]);
    REQUIRE(bitset_b[456]);

    // operator= implemented by compiler
    auto bitset_c = bitset_b;
    REQUIRE(bitset_c[1]);
    REQUIRE(bitset_c[23]);
    REQUIRE(bitset_c[456]);
}

TEST_CASE("DynamicBitset(DynamicBitset&&) moves the bitset", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    auto bitset_b = DynamicBitset(std::move(bitset));
    REQUIRE(bitset_b[1]);
    REQUIRE(bitset_b[23]);
    REQUIRE(bitset_b[456]);

    // operator= implemented by compiler
    auto bitset_c = std::move(bitset_b);
    REQUIRE(bitset_c[1]);
    REQUIRE(bitset_c[23]);
    REQUIRE(bitset_c[456]);
}

TEST_CASE("DynamicBitset() initializes all bits to 0", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    for (auto i = 0; i < 1024; i++) {
        REQUIRE_FALSE(bitset[i]);
    }
}

TEST_CASE("DynamicBitset::test() returns true iff bit is set", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);

    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    REQUIRE(bitset.test(1) == true);
    REQUIRE(bitset.test(23) == true);
    REQUIRE(bitset.test(456) == true);
}

TEST_CASE("DynamicBitset::clear() reset the selected bit", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    bitset.reset(1);
    bitset.reset(23);
    bitset.reset(456);

    REQUIRE(bitset.test(1) == false);
    REQUIRE(bitset.test(23) == false);
    REQUIRE(bitset.test(456) == false);
}

TEST_CASE("DynamicBitset::all() returns true iff all bits are set", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    for (auto i = 0; i < bitset.size(); i++) {
        bitset.set(i);
    }

    REQUIRE(bitset.all());

    bitset.reset(67);
    REQUIRE_FALSE(bitset.all());
}

TEST_CASE("DynamicBitset::any() returns true if any bits are set, false otherwise", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    REQUIRE_FALSE(bitset.any());

    bitset.set(1000);
    REQUIRE(bitset.any());
}

TEST_CASE("DynamicBitset::none() returns false if any bits are set, true otherwise", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    REQUIRE(bitset.none());

    bitset.set(1000);
    REQUIRE_FALSE(bitset.none());
}


TEST_CASE("DynamicBitset::resize() resizes bitset to next largest 64b boundary", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    REQUIRE(bitset.size() == 1024);

    bitset.resize(56);
    REQUIRE(bitset.size() == 64);

    bitset.resize(1000);
    REQUIRE(bitset.size() == 1024);
}

TEST_CASE("DynamicBitset::operator&= computes intersection of two bitsets", "[DynamicBitset]") {
    auto bitset_a = DynamicBitset(1024);
    bitset_a.set(1);
    bitset_a.set(23);
    bitset_a.set(456);

    auto bitset_b = DynamicBitset(1024);
    bitset_b.set(1);
    bitset_b.set(23);

    bitset_a &= bitset_b;

    REQUIRE(bitset_a[1]);
    REQUIRE(bitset_a[23]);
    REQUIRE_FALSE(bitset_a[456]);
}

TEST_CASE("DynamicBitset::operator& computes intersection of two bitsets", "[DynamicBitset]") {
    auto bitset_a = DynamicBitset(1024);
    bitset_a.set(1);
    bitset_a.set(23);
    bitset_a.set(456);

    auto bitset_b = DynamicBitset(1024);
    bitset_b.set(1);
    bitset_b.set(23);

    auto res = bitset_a & bitset_b;

    REQUIRE(res[1]);
    REQUIRE(res[23]);
    REQUIRE_FALSE(res[456]);
}
