#include <cstddef>

#include <catch2/catch_all.hpp>

import stacktrace;
import heap;
import fmt;

TEST_CASE("Stacktrace") {
    // heap::unseal_heap();
    SECTION("Get stacktrace") {
        // stacktrace::get_stacktrace();
        // SKIP("Not implemented!");
    }
    SECTION("Get current entry") {
        // const stacktrace::Entry& entry = stacktrace::get_current_entry();
        // SKIP("Not implemented!");
    }
    SECTION("Get entries") {
        // std::array<stacktrace::Entry, 128> entries;
        // const std::size_t writtenEntries = stacktrace::get_entries(entries.data(), entries.size(), 0);
    }
    SECTION("Print Entry") {
        // SKIP("Not implemented!");
    }
    SECTION("Print stacktrace") {
        // stacktrace::print_stacktrace(stderr);
    }
}
