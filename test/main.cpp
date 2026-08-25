#include <cstdio>

#include <catch2/catch_session.hpp>

#include "stacktrace/StackTrace.hpp"

import pican.mem;
import heap;

namespace {
void
initialize_memory_manager() {
    [[maybe_unused]]
    const auto memoryBytes = pican::mem::Manager::DEFAULT_SIZE;
    pican::mem::Manager::initialize(memoryBytes);
}
}  // namespace

int
main(int argc, char** argv) {
    heap::unseal_heap();
    initialize_memory_manager();
    stacktrace::initialize(argv);

    Catch::Session test_session{};

    int allTestsRun = test_session.run(argc, argv);

    return allTestsRun;
}
