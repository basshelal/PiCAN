#include <cstddef>

#include <catch2/catch_all.hpp>

#include "test/utils/test_utils.hpp"
#include "pican/core/core.hpp"
#include "heap/Heap.hpp"
#include "stacktrace/Stacktrace.hpp"

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
                auto* inner_data = static_cast<Data*>(userData);
                inner_data->called = true;
                inner_data->sizeBytes = sizeBytes;
            },
            &data
        );
        REQUIRE(data.called == false);
        heap::seal_heap();
        REQUIRE(heap::heap_is_sealed());
        const std::size_t allocations_count = heap::allocations_count();
        int* unused = new int;
        pican::test_utils::do_not_optimize(unused);
        CHECK(data.called);
        CHECK(data.sizeBytes == sizeof(int));
        CHECK(heap::allocations_count() == allocations_count);
        heap::unseal_heap();
        heap::reset_violation_callback();
        data.called = false;
        unused = new int;
        pican::test_utils::do_not_optimize(unused);
        CHECK(data.called == false);
        CHECK(heap::allocations_count() == (allocations_count + 1));
    }
}
