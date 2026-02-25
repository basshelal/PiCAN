#include "pican/info/MemoryReader.hpp"

#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME MemoryReader

namespace pican::info {
TEST(example) {
    Result<MemoryReader, MemoryReader::Error> memoryReaderResult = MemoryReader::create();

    ASSERT_TRUE(memoryReaderResult.is_success());

    MemoryReader memoryReader{std::move(memoryReaderResult.success_value_or_panic())};

    auto free = memoryReader.get_free_system_memory();

    ::fprintf(stderr, "free: %zu\n", free);
}
}  // namespace pican::info
