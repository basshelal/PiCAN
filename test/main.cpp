#include <cstdio>

#include <gtest/gtest.h>

#include "pican/mem/Manager.hpp"
#include "stacktrace/StackTrace.hpp"
#include "test/Heap.hpp"

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
    test::heap::unseal_heap();
    initialize_memory_manager();

    ::testing::InitGoogleTest(&argc, argv);

    stacktrace::initialize(argv);

    int allTestsRun = RUN_ALL_TESTS();

    return allTestsRun;
}
