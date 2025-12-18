#include "core/RingBuffer.hpp"
#include "core/Types.hpp"
#include "test/TestUtils.hpp"
#include "test/Tracked.hpp"

using core::RingBuffer;
using core::RingBufferOverflowBehavior;

#define TEST(testName) GTEST_TEST(RingBufferTest, testName)

using Element = Tracked<std::string>;

static constexpr Size DEFAULT_CAPACITY = 8;

template<typename TP = Element>
static constexpr auto DEFAULT_OVERFLOW_CALLBACK = RingBuffer<TP>::DEFAULT_OVERFLOW_CALLBACK;

template<typename TP = Element>
static constexpr auto DEFAULT_UNDERFLOW_CALLBACK = RingBuffer<TP>::DEFAULT_UNDERFLOW_CALLBACK;

static constexpr auto DEFAULT_OVERFLOW_BEHAVIOR = RingBufferOverflowBehavior::DEFAULT;

TEST(create) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.capacity());
    ASSERT_EQUAL(0, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_full());
    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_NULL(ringBuffer.peek());
    ASSERT_NULL(ringBuffer.pop());
}

TEST(push_copy) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    const Element element0("element0");
    ringBuffer.push_copy(element0);
    ASSERT_EQUAL(1, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_empty());

    Element* element0Ptr = ringBuffer.peek();
    ASSERT_NOT_NULL(element0Ptr);

    ASSERT_EQUAL(element0.data, element0Ptr->data);
    ASSERT_EQUAL(1, element0Ptr->copyCount);
    ASSERT_NOT_EQUAL(element0.address(), element0Ptr->address());
}

TEST(push_move) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    const std::string element0Name = "element0";
    ringBuffer.push_move(Element(element0Name));
    ASSERT_EQUAL(1, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_empty());

    Element* element0Ptr = ringBuffer.peek();
    ASSERT_EQUAL(element0Name, element0Ptr->data);
    ASSERT_EQUAL(1, element0Ptr->moveCount);
}

TEST(pop) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    const Element element0("element0");
    ringBuffer.push_copy(element0);
    ASSERT_EQUAL(1, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_empty());

    Element* element0Ptr = ringBuffer.pop();
    ASSERT_NOT_NULL(element0Ptr);
    ASSERT_EQUAL(element0.data, element0Ptr->data);

    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_EQUAL(0, ringBuffer.size());
}

TEST(clear) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    const Size elementsCount = 3;
    for (Size i = 0; i < elementsCount; ++i) {
        ringBuffer.push_move(Element{"element" + std::to_string(i)});
    }

    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.capacity());
    ASSERT_EQUAL(elementsCount, ringBuffer.size());
    ASSERT_FALSE(ringBuffer.is_empty());
    ringBuffer.clear();
    ASSERT_EQUAL(0, ringBuffer.size());
    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.capacity());
}

TEST(fill) {
    RingBuffer<Element> ringBuffer{DEFAULT_CAPACITY};

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        ringBuffer.push_move(Element{"element" + std::to_string(i)});
    }

    ASSERT_TRUE(ringBuffer.is_full());
    ASSERT_FALSE(ringBuffer.is_empty());
    ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
}

TEST(overflow) {
    Size overflowCount = 0;
    const auto overflowCallback = [&overflowCount](const RingBuffer<Element>&, const Element&) -> void {
        ++overflowCount;
    };
    RingBuffer<Element> ringBuffer{
        DEFAULT_CAPACITY, RingBufferOverflowBehavior::DEFAULT, overflowCallback,
        RingBuffer<Element>::DEFAULT_UNDERFLOW_CALLBACK
    };

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        ringBuffer.push_move(Element{"element" + std::to_string(i)});
    }
    ASSERT_TRUE(ringBuffer.is_full());
    ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
    ASSERT_EQUAL(0, overflowCount);

    const Element overwrittenElement{"overwrittenElement"};
    ringBuffer.push_copy(overwrittenElement);
    ASSERT_TRUE(ringBuffer.is_full());
    ASSERT_EQUAL(ringBuffer.capacity(), ringBuffer.size());
    ASSERT_EQUAL(DEFAULT_CAPACITY, ringBuffer.size());
    ASSERT_EQUAL(1, overflowCount);

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        Element* popped = ringBuffer.pop();
        ASSERT_NOT_NULL(popped);
        ASSERT_EQUAL("element" + std::to_string(i), popped->data);
    }
    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_EQUAL(0, ringBuffer.size());
}

TEST(underflow) {
    Size underflowCount = 0;
    const auto underflowCallback = [&underflowCount](const RingBuffer<Element>&) -> void {
        underflowCount++;
    };
    RingBuffer<Element> ringBuffer{
        DEFAULT_CAPACITY, RingBufferOverflowBehavior::DEFAULT, RingBuffer<Element>::DEFAULT_OVERFLOW_CALLBACK,
        underflowCallback
    };

    ASSERT_TRUE(ringBuffer.is_empty());
    ASSERT_EQUAL(0, ringBuffer.size());

    Element* popped = ringBuffer.pop();
    ASSERT_NULL(popped);
    ASSERT_EQUAL(1, underflowCount);
}

TEST(copy_constructor) {
    RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        originalRingBuffer.push_move(Element{"element" + std::to_string(i)});
    }
    ASSERT_TRUE(originalRingBuffer.is_full());

    RingBuffer<Element> copiedRingBuffer{originalRingBuffer};
    ASSERT_EQUAL(originalRingBuffer.capacity(), copiedRingBuffer.capacity());
    ASSERT_EQUAL(originalRingBuffer.size(), copiedRingBuffer.size());
    ASSERT_TRUE(copiedRingBuffer.is_full());

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        Element* originalPopped = originalRingBuffer.pop();
        Element* copiedPopped = copiedRingBuffer.pop();

        ASSERT_NOT_NULL(originalPopped);
        ASSERT_NOT_NULL(copiedPopped);

        ASSERT_NOT_EQUAL(originalPopped->address(), copiedPopped->address());
        ASSERT_EQUAL(originalPopped->data, copiedPopped->data);

        ASSERT_EQUAL(1, copiedPopped->copyCount);
    }
    ASSERT_TRUE(originalRingBuffer.is_empty());
    ASSERT_TRUE(copiedRingBuffer.is_empty());
}

TEST(copy_assignment) {
    RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        originalRingBuffer.push_move(Element{"element" + std::to_string(i)});
    }
    ASSERT_TRUE(originalRingBuffer.is_full());

    RingBuffer<Element> copiedRingBuffer{4};
    copiedRingBuffer = originalRingBuffer;
    ASSERT_EQUAL(originalRingBuffer.capacity(), copiedRingBuffer.capacity());
    ASSERT_EQUAL(originalRingBuffer.size(), copiedRingBuffer.size());

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        Element* originalPopped = originalRingBuffer.pop();
        Element* copiedPopped = copiedRingBuffer.pop();

        ASSERT_NOT_NULL(originalPopped);
        ASSERT_NOT_NULL(copiedPopped);

        ASSERT_NOT_EQUAL(originalPopped->address(), copiedPopped->address());
        ASSERT_EQUAL(originalPopped->data, copiedPopped->data);

        ASSERT_EQUAL(1, copiedPopped->copyCount);
    }
    ASSERT_TRUE(originalRingBuffer.is_empty());
    ASSERT_TRUE(copiedRingBuffer.is_empty());
}

TEST(move_constructor) {
    RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};

    std::vector<Element> elements;
    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        elements.emplace_back("element" + std::to_string(i));
    }
    originalRingBuffer.push_copy_all(elements);
    ASSERT_TRUE(originalRingBuffer.is_full());

    RingBuffer<Element> movedRingBuffer{std::move(originalRingBuffer)};
    ASSERT_EQUAL(DEFAULT_CAPACITY, movedRingBuffer.capacity());
    ASSERT_EQUAL(elements.size(), movedRingBuffer.size());
    ASSERT_TRUE(movedRingBuffer.is_full());

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        Element* movedPopped = movedRingBuffer.pop();
        const Element& element = elements[i];
        ASSERT_NOT_NULL(movedPopped);

        ASSERT_EQUAL(element.data, movedPopped->data);
    }
    ASSERT_TRUE(movedRingBuffer.is_empty());
}

TEST(move_assignment) {
    RingBuffer<Element> originalRingBuffer{DEFAULT_CAPACITY};

    std::vector<Element> elements;
    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        elements.emplace_back("element" + std::to_string(i));
    }
    originalRingBuffer.push_copy_all(elements);
    ASSERT_TRUE(originalRingBuffer.is_full());

    RingBuffer<Element> movedRingBuffer{4};
    movedRingBuffer = std::move(originalRingBuffer);
    ASSERT_EQUAL(DEFAULT_CAPACITY, movedRingBuffer.capacity());
    ASSERT_EQUAL(elements.size(), movedRingBuffer.size());
    ASSERT_TRUE(movedRingBuffer.is_full());

    for (Size i = 0; i < DEFAULT_CAPACITY; ++i) {
        Element* movedPopped = movedRingBuffer.pop();
        const Element& element = elements[i];
        ASSERT_NOT_NULL(movedPopped);

        ASSERT_EQUAL(element.data, movedPopped->data);
    }
    ASSERT_TRUE(movedRingBuffer.is_empty());
}
