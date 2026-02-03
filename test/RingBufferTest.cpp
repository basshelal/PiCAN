#include <string_view>

#include "pican/RingBuffer.hpp"
#include "test/Heap.hpp"
#include "test/TestUtils.hpp"
#include "test/Tracked.hpp"

#define TEST_SUITE_NAME RingBuffer

namespace pican {

using Element = Tracked<std::string_view>;
using RingBufferType = RingBuffer<Element>;

static constexpr Count DEFAULT_CAPACITY = 8;

static constexpr RingBufferOverflowBehavior DEFAULT_OVERFLOW_BEHAVIOR = RingBufferOverflowBehavior::DEFAULT;

TEST(create) {
    AUTO_HEAP_SEAL();
    const pican::Count capacity = 8;

    memory::Block block = pican::memory::Manager::get().get_block(sizeof(Element) * (capacity + 1));
    Array<Element> array{block};

    const RingBufferOverflowBehavior overflowBehavior = RingBufferOverflowBehavior::DEFAULT;

    RingBuffer<Element> ringBuffer{array, overflowBehavior};

    ASSERT_EQUAL(capacity, ringBuffer.capacity());
    ASSERT_EQUAL(overflowBehavior, ringBuffer.overflow_behavior());
    ASSERT_EQUAL(0, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_full());
    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_NULL(ringBuffer.pop_ptr());
}

TEST(push_copy) {
    AUTO_HEAP_SEAL();
    const pican::Count capacity = 8;

    memory::Block block = pican::memory::Manager::get().get_block(sizeof(Element) * (capacity + 1));
    Array<Element> array{block};

    const RingBufferOverflowBehavior overflowBehavior = RingBufferOverflowBehavior::DEFAULT;

    RingBuffer<Element> ringBuffer{array, overflowBehavior};

    ASSERT_EQUAL(capacity, ringBuffer.capacity());
    ASSERT_EQUAL(overflowBehavior, ringBuffer.overflow_behavior());
    ASSERT_EQUAL(0, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_full());
    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_NULL(ringBuffer.pop_ptr());

    const Element element{"element"};
    ringBuffer.push_copy(element);

    ASSERT_EQUAL(1, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_empty());

    Element* elementPtr = ringBuffer.pop_ptr();

    ASSERT_NOT_NULL(elementPtr);
    ASSERT_EQUAL(element.data, elementPtr->data);
    ASSERT_EQUAL(1, elementPtr->copyCount);
    ASSERT_NOT_EQUAL(element.address(), elementPtr->address());

    ASSERT_EQUAL(0, ringBuffer.size());
    ASSERT_TRUE(ringBuffer.is_empty());
}

// TEST(pop) {
//     RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};
//
//     const Element element0("element0");
//     ringBuffer.push_copy(element0);
//     ASSERT_EQUAL(1, ringBuffer.size());
//     ASSERT_FALSE(ringBuffer.is_empty());
//
//     Element* element0Ptr = ringBuffer.pop();
//     ASSERT_NOT_NULL(element0Ptr);
//     ASSERT_EQUAL(element0.data, element0Ptr->data);
//
//     ASSERT_TRUE(ringBuffer.is_empty());
//     ASSERT_EQUAL(0, ringBuffer.size());
// }
//
// TEST(clear) {
//     RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};
//
//     const Size elementsCount = 3;
//     for (Size i = 0; i < elementsCount; ++i) {
//         ringBuffer.push_move(Element{"element" + std::to_string(i)});
//     }
//
//     ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.capacity());
//     ASSERT_EQUAL(elementsCount, ringBuffer.size());
//     ASSERT_FALSE(ringBuffer.is_empty());
//     ringBuffer.clear();
//     ASSERT_EQUAL(0, ringBuffer.size());
//     ASSERT_TRUE(ringBuffer.is_empty());
//     ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.capacity());
// }
//
// TEST(fill) {
//     RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};
//
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         ringBuffer.push_move(Element{"element" + std::to_string(i)});
//     }
//
//     ASSERT_TRUE(ringBuffer.is_full());
//     ASSERT_FALSE(ringBuffer.is_empty());
//     ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
//     ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
// }
//
// TEST(overflow) {
//     Size overflowCount = 0;
//     const auto overflowCallback = [&overflowCount](const RingBuffer<Element>&, const Element&) -> void {
//         ++overflowCount;
//     };
//     RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY, RingBufferOverflowBehavior::DEFAULT, overflowCallback, nullptr};
//
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         ringBuffer.push_move(Element{"element" + std::to_string(i)});
//     }
//     ASSERT_TRUE(ringBuffer.is_full());
//     ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
//     ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
//     ASSERT_EQUAL(0, overflowCount);
//
//     const Element overwrittenElement{"overwrittenElement"};
//     ringBuffer.push_copy(overwrittenElement);
//     ASSERT_TRUE(ringBuffer.is_full());
//     ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
//     ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
//     ASSERT_EQUAL(1, overflowCount);
//
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         Element* popped = ringBuffer.pop();
//         ASSERT_NOT_NULL(popped);
//         ASSERT_EQUAL("element" + std::to_string(i), popped->data);
//     }
//     ASSERT_TRUE(ringBuffer.is_empty());
//     ASSERT_EQUAL(0, ringBuffer.size());
// }
//
// TEST(underflow) {
//     Size underflowCount = 0;
//     const auto underflowCallback = [&underflowCount](const RingBuffer<Element>&) -> void {
//         underflowCount++;
//     };
//     RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY, RingBufferOverflowBehavior::DEFAULT, nullptr,
//     underflowCallback};
//
//     ASSERT_TRUE(ringBuffer.is_empty());
//     ASSERT_EQUAL(0, ringBuffer.size());
//
//     Element* popped = ringBuffer.pop();
//     ASSERT_NULL(popped);
//     ASSERT_EQUAL(1, underflowCount);
// }
//
// TEST(move_constructor) {
//     RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};
//
//     std::vector<Element> elements;
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         elements.emplace_back("element" + std::to_string(i));
//         originalRingBuffer.push_copy(elements[i]);
//     }
//
//     ASSERT_TRUE(originalRingBuffer.is_full());
//
//     RingBuffer<Element> movedRingBuffer{std::move(originalRingBuffer)};
//     ASSERT_EQUAL(DEFAULT_CAPACITY, movedRingBuffer.capacity());
//     ASSERT_EQUAL(elements.size(), movedRingBuffer.size());
//     ASSERT_TRUE(movedRingBuffer.is_full());
//
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         Element* movedPopped = movedRingBuffer.pop();
//         const Element& element = elements[i];
//         ASSERT_NOT_NULL(movedPopped);
//
//         ASSERT_EQUAL(element.data, movedPopped->data);
//     }
//     ASSERT_TRUE(movedRingBuffer.is_empty());
// }
//
// TEST(move_assignment) {
//     RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};
//
//     std::vector<Element> elements;
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         elements.emplace_back("element" + std::to_string(i));
//         originalRingBuffer.push_copy(elements[i]);
//     }
//     ASSERT_TRUE(originalRingBuffer.is_full());
//
//     RingBuffer<Element> movedRingBuffer{4};
//     movedRingBuffer = std::move(originalRingBuffer);
//     ASSERT_EQUAL(DEFAULT_CAPACITY, movedRingBuffer.capacity());
//     ASSERT_EQUAL(elements.size(), movedRingBuffer.size());
//     ASSERT_TRUE(movedRingBuffer.is_full());
//
//     for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
//         Element* movedPopped = movedRingBuffer.pop();
//         const Element& element = elements[i];
//         ASSERT_NOT_NULL(movedPopped);
//
//         ASSERT_EQUAL(element.data, movedPopped->data);
//     }
//     ASSERT_TRUE(movedRingBuffer.is_empty());
// }
}  // namespace pican
