#include <cstdio>

#include <catch2/catch_session.hpp>

#include "pican/mem/mem.hpp"
#include "heap/Heap.hpp"
#include "stacktrace/Stacktrace.hpp"

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
    stacktrace::initialize(argv);
    heap::unseal_heap();
    initialize_memory_manager();

    Catch::Session testSession{};

    int allTestsRun = testSession.run(argc, argv);

    return allTestsRun;
}
