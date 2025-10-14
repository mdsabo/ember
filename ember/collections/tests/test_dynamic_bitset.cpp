#include <catch2/catch_test_macros.hpp>

#include "DynamicBitset.h"

using namespace ember::collections;

TEST_CASE("DynamicBitset() creates new bitset with optional size", "[DynamicBitset]") {
    auto zero_size = DynamicBitset();
    REQUIRE(zero_size.size() == 0);

    auto bitset = DynamicBitset(1024);
    REQUIRE(bitset.size() == 1024);
}

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
    auto bitset = DynamicBitset(4);
    for (auto i = 0; i < 4; i++) {
        REQUIRE_FALSE(bitset[i]);
    }
}

TEST_CASE("DynamicBitset::test() returns true iff bit is in set", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);

    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    REQUIRE_FALSE(bitset.test(0));
    REQUIRE(bitset.test(1));
    REQUIRE(bitset.test(23));
    REQUIRE(bitset.test(456));
    REQUIRE_FALSE(bitset.test(1025));
}

TEST_CASE("DynamicBitset::clear() reset the selected bit", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);
    bitset.set(1);
    bitset.set(23);
    bitset.set(456);

    bitset.reset(1);
    bitset.reset(23);
    bitset.reset(456);

    REQUIRE_FALSE(bitset.test(1));
    REQUIRE_FALSE(bitset.test(23));
    REQUIRE_FALSE(bitset.test(456));
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


TEST_CASE("DynamicBitset::resize() sets bitset size", "[DynamicBitset]") {
    auto bitset = DynamicBitset(1024);

    bitset.resize(56);
    REQUIRE(bitset.size() == 56);

    bitset.resize(1000);
    REQUIRE(bitset.size() == 1000);
}

TEST_CASE("DynamicBitset::resize() resets bits", "[DynamicBitset]") {
    auto bitset = DynamicBitset(64);
    bitset.set(60);

    bitset.resize(56);
    bitset.resize(128);

    REQUIRE_FALSE(bitset[60]);
    REQUIRE_FALSE(bitset[100]);
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
