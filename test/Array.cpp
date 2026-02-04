#include <string_view>

#include "pican/RingBuffer.hpp"
#include "test/Heap.hpp"
#include "test/TestUtils.hpp"
#include "test/Tracked.hpp"

#define TEST_SUITE_NAME Array

namespace pican {
using Element = Tracked<std::string_view>;
constexpr SizeBytes ELEMENT_SIZE = sizeof(Element);

TEST(create) {
    AUTO_HEAP_SEAL();

    Count count = 8;
    SizeBytes size = ELEMENT_SIZE * 4;

    mem::Block block = mem::Manager::get().get_block(size);

    Array<Element> array{block};

    ASSERT_EQUAL(count, array.items_count());
    ASSERT_EQUAL(block.address(), array.block().address());
    ASSERT_EQUAL(block.size_bytes(), array.block().size_bytes());
    ASSERT_EQUAL(count - 1, array.last_index());
}

TEST(set_and_get) {
    AUTO_HEAP_SEAL();

    Count count = 8;
    SizeBytes size = ELEMENT_SIZE * count;

    mem::Block block = mem::Manager::get().get_block(size);

    Array<Element> array{block};

    Element element0{"element0"};
    array.set_at(0, element0);

    const Element& gotElement0 = array.get_at(0);
    ASSERT_EQUAL(element0.data, gotElement0.data);
    ASSERT_EQUAL(element0.moveCount, 0);
    ASSERT_EQUAL(element0.copyCount, 0);
    ASSERT_EQUAL(gotElement0.moveCount, 0);
    ASSERT_EQUAL(gotElement0.copyCount, 1);
    ASSERT_EQUAL(gotElement0.lastOperation, LifetimeOperation::COPY_ASSIGNMENT);
}

// TEST(invalid_set_and_get) {
//     AUTO_HEAP_SEAL();
//
//     Count count = 8;
//     SizeBytes size = ELEMENT_SIZE * count;
//
//     memory::Block block = memory::Manager::get().get_block(size);
//
//     Array<Element> array{block};
//
//     Element element0{"element0"};
//     array.set_at(count + 1, element0);
//
//     const Element& gotElement0 = array.get_at(0);
//     ASSERT_EQUAL(element0.data, gotElement0.data);
//     ASSERT_EQUAL(element0.moveCount, 0);
//     ASSERT_EQUAL(element0.copyCount, 0);
//     ASSERT_EQUAL(gotElement0.moveCount, 0);
//     ASSERT_EQUAL(gotElement0.copyCount, 1);
//     ASSERT_EQUAL(gotElement0.lastOperation, LifetimeOperation::COPY_ASSIGNMENT);
// }

}  // namespace pican
