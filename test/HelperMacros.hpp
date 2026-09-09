#ifndef PICAN_TEST_HELPERMACROS
#define PICAN_TEST_HELPERMACROS

#include <catch2/catch_all.hpp>

#define CHECK_WITH_VARIABLE(variable, ...) \
    CHECKED_IF(__VA_ARGS__) {              \
        variable &= true;                  \
    }

#endif  // PICAN_TEST_HELPERMACROS
