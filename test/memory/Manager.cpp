#include "pican/memory/Manager.hpp"

#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME MemoryManager

namespace pican::memory {

TEST(initialize) {
    Manager::initialize(100);
}
}  // namespace pican::memory
