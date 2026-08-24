#include "catch2/catch_all.hpp"
#include "test/Tracked.hpp"

using LifeTimeCallback = Tracked<std::string>::LifetimeCallback;
using LifeTimeCallbacks = Tracked<std::string>::LifetimeCallbacks;

TEST_CASE("Tracked") {
    const std::string data{"element"};
    bool called = false;

    const LifeTimeCallback default_callback = [&called](const auto&) {
        called = true;
    };
    CHECK_FALSE(called);

    SECTION("Parameterized constructor") {
        const LifeTimeCallbacks callbacks{.onConstructor = default_callback};
        const Tracked<std::string> tracked{data, callbacks};

        CHECK(tracked.data == data);
        CHECK(tracked.lastOperation == LifetimeOperation::CONSTRUCTOR);
        CHECK(tracked.copyCount == 0);
        CHECK(tracked.moveCount == 0);
        CHECK(called);
    }

    SECTION("Copy constructor") {
        const LifeTimeCallbacks callbacks{.onCopyConstructor = default_callback};
        const Tracked<std::string> original{"element", callbacks};
        CHECK_FALSE(called);
        const Tracked<std::string> copy{original};

        CHECK(called);
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
        const LifeTimeCallbacks callbacks{.onCopyAssignment = default_callback};
        const Tracked<std::string> original{"element", callbacks};
        Tracked<std::string> copy{};
        CHECK_FALSE(called);
        copy = original;

        CHECK(called);
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
        const LifeTimeCallbacks callbacks{.onMoveConstructor = default_callback};
        Tracked<std::string> original{"element", callbacks};
        CHECK_FALSE(called);
        const Tracked<std::string> copy{std::move(original)};

        CHECK(called);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(copy.copyCount == 0);
        CHECK(copy.moveCount == 1);
    }

    SECTION("Move assignment") {
        const LifeTimeCallbacks callbacks{.onMoveAssignment = default_callback};
        Tracked<std::string> original{"element", callbacks};
        Tracked<std::string> copy{};
        CHECK_FALSE(called);
        copy = std::move(original);

        CHECK(called);
        CHECK(copy.data == data);
        CHECK(copy.lastOperation == LifetimeOperation::MOVE_ASSIGNMENT);
        CHECK(copy.copyCount == 0);
        CHECK(copy.moveCount == 1);
    }
}
