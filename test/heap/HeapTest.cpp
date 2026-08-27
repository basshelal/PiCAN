#include <cstddef>

#include <catch2/catch_all.hpp>
import heap;

TEST_CASE("Heap") {
    SECTION("Seal and unseal") {
        heap::set_heap_sealed_callback([]() {
            printf("ERR!\n");
        });
        [[maybe_unused]]
        const std::size_t allocations_count = heap::allocations_count();
        heap::seal_heap();
        REQUIRE(heap::heap_is_sealed());
        [[maybe_unused]]
        auto s = new int;
    }
}
