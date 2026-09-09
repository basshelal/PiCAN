#include "catch2/catch_all.hpp"

import pican.test_utils;

using pican::test_utils::LifetimeOperation;
using Tracked = pican::test_utils::Tracked<std::string>;

using LifeTimeCallback = Tracked::LifetimeCallback;
using LifeTimeCallbacks = Tracked::LifetimeCallbacks;

TEST_CASE("Tracked") {
    const std::string data{"element"};
    int callbackCalledCount = 0;
    const LifeTimeCallback defaultCallback = [&callbackCalledCount](const auto&) {
        callbackCalledCount++;
    };
    CHECK(callbackCalledCount == 0);
    int destructorCalledCount = 0;
    const LifeTimeCallback onDestructed = [&destructorCalledCount](const auto&) {
        destructorCalledCount++;
    };
    CHECK(destructorCalledCount == 0);

    SECTION("Parameterized constructor") {
        const LifeTimeCallbacks callbacks{.onConstructed = defaultCallback, .onDestructed = onDestructed};
        const Tracked tracked{data, callbacks};

        CHECK(tracked.data == data);
        CHECK(tracked.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(tracked.copyCount == 0);
        CHECK(tracked.moveCount == 0);
        CHECK(callbackCalledCount == 1);
        tracked.~Tracked();
        CHECK(destructorCalledCount == 1);
    }

    SECTION("Copy constructor") {
        const LifeTimeCallbacks callbacks{.onCopyConstructed = defaultCallback};
        const Tracked original{"element", callbacks};
        CHECK(callbackCalledCount == 0);
        const Tracked copy{original};

        CHECK(callbackCalledCount == 1);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
        CHECK(copy.copyCount == 1);
        CHECK(copy.moveCount == 0);

        CHECK(original.data == data);
        CHECK(original.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(original.copyCount == 0);
        CHECK(original.moveCount == 0);
    }

    SECTION("Copy assignment") {
        const LifeTimeCallbacks callbacks{.onCopyAssigned = defaultCallback};
        const Tracked original{"element", callbacks};
        Tracked copy{};
        CHECK(callbackCalledCount == 0);
        copy = original;

        CHECK(callbackCalledCount == 1);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::COPY_ASSIGNMENT);
        CHECK(copy.copyCount == 1);
        CHECK(copy.moveCount == 0);

        CHECK(original.data == data);
        CHECK(original.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(original.copyCount == 0);
        CHECK(original.moveCount == 0);
    }

    SECTION("Move constructor") {
        const LifeTimeCallbacks callbacks{.onMoveConstructed = defaultCallback, .onDestructed = onDestructed};
        Tracked original{"element", callbacks};
        CHECK(callbackCalledCount == 0);
        const Tracked copy{std::move(original)};

        CHECK(callbackCalledCount == 1);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(copy.copyCount == 0);
        CHECK(copy.moveCount == 1);
        copy.~Tracked();
        CHECK(destructorCalledCount == 1);
    }

    SECTION("Move assignment") {
        const LifeTimeCallbacks callbacks{.onMoveAssigned = defaultCallback, .onDestructed = onDestructed};
        Tracked original{"element", callbacks};
        Tracked copy{};
        CHECK(callbackCalledCount == 0);
        copy = std::move(original);

        CHECK(destructorCalledCount == 0);
        CHECK(callbackCalledCount == 1);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::MOVE_ASSIGNMENT);
        CHECK(copy.copyCount == 0);
        CHECK(copy.moveCount == 1);
        copy.~Tracked();
        CHECK(destructorCalledCount == 1);
    }
}
