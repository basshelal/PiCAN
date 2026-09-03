#include <cstddef>

#include <catch2/catch_all.hpp>
import heap;
import stacktrace;

TEST_CASE("Heap") {
    SECTION("Seal and unseal") {
        // TODO(bxh) 02-Sep-26 23:46 Need a way to pass in data, use C style callbacks or allow C++ lambdas?
        struct Data {
            bool called = false;
        };

        Data data;
        heap::set_violation_callback(
            [](void* userData) -> void {
                auto* data = static_cast<Data*>(userData);
                const auto& entry = stacktrace::get_current_entry();
                stacktrace::print_entry(entry, stderr);
                data->called = true;
            },
            &data
        );
        [[maybe_unused]]
        const std::size_t allocations_count = heap::allocations_count();
        heap::seal_heap();
        REQUIRE(heap::heap_is_sealed());
        [[maybe_unused]]
        auto s = new int;
        REQUIRE(data.called);
        heap::unseal_heap();
        heap::set_violation_callback(nullptr, nullptr);
    }
}
