#include <array>
#include <catch2/catch_test_macros.hpp>

#include "Allocators.h"

using namespace ember::util;

struct TestObject {
    TestObject() {
        data = 0xA5;
    }
    ~TestObject() {
        data = 0x0;
    }
    uint64_t data;
};

TEST_CASE("SlabAllocator::malloc() returns pointer to pre-initialized allocated object", "[Allocators]") {
    auto allocator = SlabAllocator<TestObject>();
    auto obj = allocator.malloc();
    REQUIRE(obj->data == 0xA5);
}

TEST_CASE("SlabAllocator::free() destroys object and returns it to the pool re-initialized", "[Allocators]") {
    auto allocator = SlabAllocator<TestObject>();
    auto obj = allocator.malloc();

    obj->data = 10;
    allocator.free(obj);

    REQUIRE(obj->data == 0xA5);
    REQUIRE(allocator.malloc() == obj);
}