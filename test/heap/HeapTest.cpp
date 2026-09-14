#include <cstddef>

#include <catch2/catch_all.hpp>

import pican.core;
import heap;
import stacktrace;

struct Data {
    bool called = false;
    std::size_t sizeBytes = 0;
};

TEST_CASE("Heap") {
    SECTION("Seal and Unseal") {
        Data data;
        REQUIRE(data.called == false);
        REQUIRE(data.sizeBytes == 0);
        heap::set_violation_callback(
            [](std::size_t sizeBytes, void* userData) -> void {
                auto* data = static_cast<Data*>(userData);
                data->called = true;
                data->sizeBytes = sizeBytes;
            },
            &data
        );
        REQUIRE(data.called == false);
        heap::seal_heap();
        REQUIRE(heap::heap_is_sealed());
        const std::size_t allocations_count = heap::allocations_count();
        [[maybe_unused]]
        int* unused = new int;
        REQUIRE(data.called);
        REQUIRE(data.sizeBytes == sizeof(int));
        REQUIRE(heap::allocations_count() == allocations_count);
        heap::unseal_heap();
        heap::reset_violation_callback();
        data.called = false;
        unused = new int;
        REQUIRE(data.called == false);
        REQUIRE(heap::allocations_count() == (allocations_count + 1));
    }
}
