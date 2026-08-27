#include <cstddef>

#include <catch2/catch_all.hpp>
import stacktrace;

TEST_CASE("Stacktrace") {
    SECTION("Print") {
        stacktrace::print_stacktrace(stderr);
    }
}
