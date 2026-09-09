#include <cstdio>

#include <catch2/catch_session.hpp>

import pican.mem;
import pican.heap;
import pican.trace;

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
    pican::trace::initialize(argv);
    pican::heap::unseal_heap();
    initialize_memory_manager();

    Catch::Session testSession{};

    int allTestsRun = testSession.run(argc, argv);

    return allTestsRun;
}
