#include "core/Types.hpp"
#include "test/TestUtils.hpp"
#include "test/Tracked.hpp"

#define TEST(testName) GTEST_TEST(TrackedTest, testName)

TEST(argument_constructor) {
    const std::string data{"element"};
    const Tracked<std::string> tracked{data};

    ASSERT_EQUAL(data, tracked.data);
    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, tracked.lastOperation);
    ASSERT_EQUAL(0, tracked.copyCount);
    ASSERT_EQUAL(0, tracked.moveCount);
}

TEST(copy_constructor) {
    const std::string data{"element"};
    const Tracked<std::string> original{"element"};
    const Tracked<std::string> copy{original};

    ASSERT_EQUAL(data, copy.data);
    ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, copy.lastOperation);
    ASSERT_EQUAL(1, copy.copyCount);
    ASSERT_EQUAL(0, copy.moveCount);

    ASSERT_EQUAL(data, original.data);
    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, original.lastOperation);
    ASSERT_EQUAL(0, original.copyCount);
    ASSERT_EQUAL(0, original.moveCount);
}

TEST(copy_assignment) {
    const std::string data{"element"};
    const Tracked<std::string> original{"element"};
    Tracked<std::string> copy{};
    copy = original;

    ASSERT_EQUAL(data, copy.data);
    ASSERT_EQUAL(LifetimeOperation::COPY_ASSIGNMENT, copy.lastOperation);
    ASSERT_EQUAL(1, copy.copyCount);
    ASSERT_EQUAL(0, copy.moveCount);

    ASSERT_EQUAL(data, original.data);
    ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, original.lastOperation);
    ASSERT_EQUAL(0, original.copyCount);
    ASSERT_EQUAL(0, original.moveCount);
}

TEST(move_constructor) {
    const std::string data{"element"};
    Tracked<std::string> original{"element"};
    const Tracked<std::string> copy{std::move(original)};

    ASSERT_EQUAL(data, copy.data);
    ASSERT_EQUAL(LifetimeOperation::MOVE_CONSTRUCTOR, copy.lastOperation);
    ASSERT_EQUAL(0, copy.copyCount);
    ASSERT_EQUAL(1, copy.moveCount);
}

TEST(move_assignment) {
    const std::string data{"element"};
    Tracked<std::string> original{"element"};
    Tracked<std::string> copy{};
    copy = std::move(original);

    ASSERT_EQUAL(data, copy.data);
    ASSERT_EQUAL(LifetimeOperation::MOVE_ASSIGNMENT, copy.lastOperation);
    ASSERT_EQUAL(0, copy.copyCount);
    ASSERT_EQUAL(1, copy.moveCount);
}
