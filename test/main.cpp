#include <cstdio>

#include <gtest/gtest.h>

#include "pican/memory/Manager.hpp"
#include "stacktrace/StackTrace.hpp"
#include "test/Heap.hpp"

namespace {
void
initialize_memory_manager() {
    [[maybe_unused]]
    const auto memoryBytes = pican::memory::Manager::DEFAULT_SIZE;
    pican::memory::Manager::initialize(memoryBytes);
    [[maybe_unused]]
    pican::memory::Manager& memoryManager = pican::memory::Manager::get();
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
