#include <string>
#include <utility>

#include <catch2/catch_all.hpp>

#include "pican/core/core.hpp"
#include "test/utils/test_utils.hpp"

using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;

using ValueType = Tracked<std::string>;
using Option = pican::Option<ValueType>;

namespace {

// Unlike Result, where every creation method exists for both success and failure, a none has only one way to be
// created, so NONE is its own creation method rather than a separate isSome flag. That way every generated
// combination is meaningful and none isn't tested 4 identical times
enum class CreationMethod : std::uint8_t {
    BY_COPY,
    BY_MOVE,
    EMPLACE,
    DEFAULT,
    NONE,
};

std::string
to_string(CreationMethod method) {
    switch (method) {
        case CreationMethod::BY_COPY:
            return "By Copy";
        case CreationMethod::BY_MOVE:
            return "By Move";
        case CreationMethod::EMPLACE:
            return "Emplace";
        case CreationMethod::DEFAULT:
            return "Default";
        case CreationMethod::NONE:
            return "None";
    }

    return "Unknown";
}

// Checks that the value's last lifetime operation is the one its creation method should have caused, e.g. a value
// created by some_by_copy must have been copy constructed (or copy assigned, if isAssignment) and nothing else
bool
check_matches(CreationMethod method, LifetimeOperation operation, bool isAssignment) {
    switch (method) {
        case CreationMethod::BY_COPY: {
            if (isAssignment) {
                CHECKED_ELSE(operation == LifetimeOperation::COPY_ASSIGNMENT) {
                    return false;
                }
            } else {
                CHECKED_ELSE(operation == LifetimeOperation::COPY_CONSTRUCTOR) {
                    return false;
                }
            }
            return true;
        }
        case CreationMethod::BY_MOVE: {
            if (isAssignment) {
                CHECKED_ELSE(operation == LifetimeOperation::MOVE_ASSIGNMENT) {
                    return false;
                }
            } else {
                CHECKED_ELSE(operation == LifetimeOperation::MOVE_CONSTRUCTOR) {
                    return false;
                }
            }
            return true;
        }
        case CreationMethod::EMPLACE:
        case CreationMethod::DEFAULT: {
            if (isAssignment) {
                FAIL("Impossible state " << to_string(method) << " and isAssignment");
                return false;
            }
            CHECKED_ELSE(operation == LifetimeOperation::CONSTRUCTOR) {
                return false;
            }
            return true;
        }
        case CreationMethod::NONE: {
            // A none has no value, so there's no lifetime operation to check, reaching here is a bug in the test
            FAIL("Impossible state " << to_string(method) << " has no value to check");
            return false;
        }
    }
    pican::panic("Unreachable");
}

Option
create_option(CreationMethod method, const std::string& data) {
    switch (method) {
        case CreationMethod::BY_COPY: {
            const ValueType v{data};
            return Option::some_by_copy(v);
        }
        case CreationMethod::BY_MOVE: {
            ValueType v{data};
            return Option::some_by_move(std::move(v));
        }
        case CreationMethod::EMPLACE: {
            return Option::some_emplace(data);
        }
        case CreationMethod::DEFAULT: {
            return Option::some_default();
        }
        case CreationMethod::NONE: {
            return Option::none();
        }
    }
    pican::panic("Unreachable");
}

}  // namespace

// Compile time checks. These are static_asserts, so if one fails the test binary doesn't build, which is the point:
// they check the parts of Option's contract that are about what code compiles rather than what it does at runtime.
// Each positive check (static_assert(Can...)) sits next to its negative ones so that a typo in a concept, which would
// make it always false, can't make the negative checks pass by accident
namespace {

// Only movable, to check that Option's copy operations disappear when the value type can't be copied
struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly& rhs) = delete;
    MoveOnly(MoveOnly&& rhs) noexcept = default;
    MoveOnly& operator=(const MoveOnly& rhs) = delete;
    MoveOnly& operator=(MoveOnly&& rhs) noexcept = default;
    ~MoveOnly() = default;
};

// Movable but not noexcept, to check that Option's noexcept follows the value type's noexcept
struct PotentiallyThrowingMove {
    PotentiallyThrowingMove() = default;
    PotentiallyThrowingMove(const PotentiallyThrowingMove& rhs) = default;

    PotentiallyThrowingMove(PotentiallyThrowingMove&& rhs) noexcept(false) {
    }

    PotentiallyThrowingMove& operator=(const PotentiallyThrowingMove& rhs) = default;

    PotentiallyThrowingMove&
    operator=(PotentiallyThrowingMove&& rhs) noexcept(false) {
        return *this;
    }

    ~PotentiallyThrowingMove() = default;
};

// No default constructor, to check that none() never needs to construct a value
struct NotDefaultConstructible {
    explicit NotDefaultConstructible(int v) : value(v) {
    }

    int value;
};

// Copy-control availability and noexcept follow the value type
static_assert(std::is_copy_constructible_v<Option>);
static_assert(std::is_copy_assignable_v<Option>);
static_assert(std::is_nothrow_move_constructible_v<Option>);
static_assert(std::is_nothrow_move_assignable_v<Option>);

using MoveOnlyOption = pican::Option<MoveOnly>;
static_assert(!std::is_copy_constructible_v<MoveOnlyOption>);
static_assert(!std::is_copy_assignable_v<MoveOnlyOption>);
static_assert(std::is_nothrow_move_constructible_v<MoveOnlyOption>);
static_assert(std::is_nothrow_move_assignable_v<MoveOnlyOption>);

using PotentiallyThrowingMoveOption = pican::Option<PotentiallyThrowingMove>;
static_assert(std::is_move_constructible_v<PotentiallyThrowingMoveOption>);
static_assert(std::is_move_assignable_v<PotentiallyThrowingMoveOption>);
static_assert(!std::is_nothrow_move_constructible_v<PotentiallyThrowingMoveOption>);
static_assert(!std::is_nothrow_move_assignable_v<PotentiallyThrowingMoveOption>);

// An Option is never default constructed or implicitly created from a value, only the factory functions create one
static_assert(!std::is_default_constructible_v<Option>);
static_assert(!std::is_constructible_v<Option, ValueType>);
static_assert(!std::is_convertible_v<ValueType, Option>);

// Each concept below is true if the expression inside it compiles. Because a concept is a template, an invalid
// expression inside it (such as calling a deleted function) makes it false rather than being a compile error, which
// is what lets us static_assert that something does NOT compile. The requires parameters are never constructed, they
// only name objects of the given type and value category for the expressions to use

template<typename Option_TP>
concept CanValueOrElse = requires(Option_TP option, typename Option_TP::ValueType defaultValue) {
    option.value_or_else(defaultValue);
};

template<typename Option_TP>
concept CanValueOrElseWithTemporaryDefault = requires(Option_TP option, typename Option_TP::ValueType defaultValue) {
    option.value_or_else(std::move(defaultValue));
};

template<typename Option_TP>
concept CanValueOrElseOnTemporary = requires(Option_TP option, typename Option_TP::ValueType defaultValue) {
    std::move(option).value_or_else(defaultValue);
};

template<typename Option_TP>
concept CanValueOrElseOnConstTemporary = requires(const Option_TP option, typename Option_TP::ValueType defaultValue) {
    std::move(option).value_or_else(defaultValue);
};

template<typename Option_TP>
concept CanValueOrPanic = requires(Option_TP option) { option.value_or_panic(); };

template<typename Option_TP>
concept CanValueOrPanicOnTemporary = requires(Option_TP option) { std::move(option).value_or_panic(); };

template<typename Option_TP>
concept CanValueExtractOrPanic = requires(Option_TP option) { option.value_extract_or_panic(); };

template<typename Option_TP>
concept CanValueExtractOrPanicOnTemporary = requires(Option_TP option) { std::move(option).value_extract_or_panic(); };

// value_or_else must never return a reference to a temporary, whether that's the default or the Option itself
static_assert(CanValueOrElse<Option>);
static_assert(!CanValueOrElseWithTemporaryDefault<Option>);
static_assert(!CanValueOrElseOnTemporary<Option>);
static_assert(!CanValueOrElseOnConstTemporary<Option>);

// value_or_panic returns a reference so it's lvalue only, extract is its rvalue only counterpart
static_assert(CanValueOrPanic<Option>);
static_assert(!CanValueOrPanicOnTemporary<Option>);
static_assert(!CanValueExtractOrPanic<Option>);
static_assert(CanValueExtractOrPanicOnTemporary<Option>);

}  // namespace

TEST_CASE("Option") {
    const CreationMethod creationMethod = GENERATE(CreationMethod::BY_COPY,
                                                   CreationMethod::BY_MOVE,
                                                   CreationMethod::EMPLACE,
                                                   CreationMethod::DEFAULT,
                                                   CreationMethod::NONE);
    const bool isSome = creationMethod != CreationMethod::NONE;
    const bool isNone = !isSome;

    DYNAMIC_SECTION("Creation: " << to_string(creationMethod)) {
        const std::string data = "some_data";
        const std::string defaultData = "default_data";
        std::string expectedData;
        if (creationMethod == CreationMethod::DEFAULT) {
            expectedData = std::string{};
        } else {
            expectedData = data;
        }

        // non const to allow for mutable refs pointing to it
        ValueType defaultValue{defaultData};

        Option optionMut = create_option(creationMethod, data);
        CHECK(optionMut.has_value() == isSome);
        CHECK(optionMut.is_empty() == isNone);

        const Option optionConst = create_option(creationMethod, data);
        CHECK(optionConst.has_value() == isSome);
        CHECK(optionConst.is_empty() == isNone);

        SECTION("value_or_else") {
            ValueType& defaultRef = defaultValue;
            ValueType& ref = optionMut.value_or_else(defaultRef);
            if (isSome) {
                CHECK(ref.data == expectedData);
                CHECK_FALSE(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(check_matches(creationMethod, ref.lastOperation, false));
            } else {
                CHECK(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(ref.data == defaultData);
                CHECK(ref.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }

            const ValueType& defaultRefConst = defaultValue;
            const ValueType& refConst = optionConst.value_or_else(defaultRefConst);
            if (isSome) {
                CHECK(refConst.data == expectedData);
                CHECK_FALSE(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            } else {
                CHECK(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(refConst.data == defaultData);
                CHECK(refConst.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }
        }

        // The panicking paths (calling these on a none) aren't tested, a panic aborts the whole test process
        if (isSome) {
            SECTION("value_or_panic") {
                ValueType& ref = optionMut.value_or_panic();
                CHECK(ref.data == expectedData);
                CHECK(check_matches(creationMethod, ref.lastOperation, false));

                const ValueType& refConst = optionConst.value_or_panic();
                CHECK(refConst.data == expectedData);
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            }

            SECTION("value_extract_or_panic") {
                const ValueType extracted = std::move(optionMut).value_extract_or_panic();
                CHECK(extracted.data == expectedData);
                CHECK(extracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                // Extracting moves the value out but doesn't change the Option's state, it's still some holding a
                // moved-from value, reading is_some() after the move is deliberate
                CHECK(optionMut.has_value());
            }
        }

        SECTION("Copy Constructor") {
            const Option copy{optionMut};
            CHECK(copy.has_value() == isSome);

            if (isSome) {
                CHECK(copy.value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                CHECK(copy.value_or_panic().data == expectedData);
                // A copy must leave its source untouched
                CHECK(optionMut.value_or_panic().data == expectedData);
            }
        }

        SECTION("Move Constructor") {
            const Option moved{std::move(optionMut)};
            CHECK(moved.has_value() == isSome);

            if (isSome) {
                CHECK(moved.value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                CHECK(moved.value_or_panic().data == expectedData);
                // Moving doesn't change the source's state, only its value is moved from, reading is_some() after the
                // move is deliberate
                CHECK(optionMut.has_value());
            }
        }

        SECTION("Self Assignment") {
            // Through references so the compiler doesn't warn about the obvious self assignment, the point is to hit
            // the this == &rhs early return, so nothing should change, not even lastOperation
            const Option& selfConst = optionMut;
            optionMut = selfConst;
            CHECK(optionMut.has_value() == isSome);
            if (isSome) {
                CHECK(optionMut.value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, optionMut.value_or_panic().lastOperation, false));
            }

            Option& selfMut = optionMut;
            optionMut = std::move(selfMut);
            CHECK(optionMut.has_value() == isSome);
            if (isSome) {
                CHECK(optionMut.value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, optionMut.value_or_panic().lastOperation, false));
            }
        }

        const bool assignFromSome = GENERATE(true, false);
        DYNAMIC_SECTION("Assignment from: " << (assignFromSome ? "Some" : "None")) {
            const std::string assignData = "assign_data";
            Option assignSource =
                create_option(assignFromSome ? CreationMethod::EMPLACE : CreationMethod::NONE, assignData);

            // When both are some, our existing value is assigned to. When we're none there's no live value to assign
            // to, so a new one is constructed from the source's value instead. When the source is none, ours (if any)
            // is destroyed, which the "Option lifetimes are balanced" test case below checks
            SECTION("Copy Assignment") {
                optionMut = assignSource;
                const LifetimeOperation expectedOperation =
                    isSome ? LifetimeOperation::COPY_ASSIGNMENT : LifetimeOperation::COPY_CONSTRUCTOR;

                CHECK(optionMut.has_value() == assignFromSome);
                if (assignFromSome) {
                    const ValueType& refConst = optionMut.value_or_panic();
                    CHECK(refConst.data == assignData);
                    CHECK(refConst.lastOperation == expectedOperation);
                    // A copy must leave its source untouched
                    CHECK(assignSource.value_or_panic().data == assignData);
                }
            }

            SECTION("Move Assignment") {
                optionMut = std::move(assignSource);
                const LifetimeOperation expectedOperation =
                    isSome ? LifetimeOperation::MOVE_ASSIGNMENT : LifetimeOperation::MOVE_CONSTRUCTOR;

                CHECK(optionMut.has_value() == assignFromSome);
                if (assignFromSome) {
                    const ValueType& refConst = optionMut.value_or_panic();
                    CHECK(refConst.data == assignData);
                    CHECK(refConst.lastOperation == expectedOperation);
                }
            }
        }
    }
}

// Checks that Option starts and ends the lifetime of its value exactly once. aliveCount goes up whenever a Tracked is
// constructed (in any way) and down whenever one is destroyed, so if Option ever forgets to destroy its value, destroys
// it twice, or destroys a value it never constructed, the count ends up wrong
TEST_CASE("Option lifetimes are balanced") {
    int aliveCount = 0;
    const ValueType::LifetimeCallback onStarted = [&aliveCount](const ValueType&) {
        aliveCount++;
    };
    const ValueType::LifetimeCallback onEnded = [&aliveCount](const ValueType&) {
        aliveCount--;
    };
    const ValueType::LifetimeCallbacks callbacks{
        .onConstructed = onStarted,
        .onCopyConstructed = onStarted,
        .onMoveConstructed = onStarted,
        .onDestructed = onEnded,
    };

    SECTION("Some is destroyed with its Option") {
        {
            const Option option = Option::some_emplace(std::string{"data"}, callbacks);
            CHECK(aliveCount == 1);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("None never constructs or destroys a value") {
        {
            const Option option = Option::none();
            CHECK(aliveCount == 0);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Copies and moves are all destroyed") {
        {
            Option original = Option::some_emplace(std::string{"data"}, callbacks);
            const Option copy{original};
            CHECK(aliveCount == 2);
            // The moved-from original still holds a (moved-from) value, so it still counts as alive
            const Option moved{std::move(original)};
            CHECK(aliveCount == 3);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Assigning none over some destroys the value") {
        {
            Option option = Option::some_emplace(std::string{"data"}, callbacks);
            const Option none = Option::none();
            CHECK(aliveCount == 1);

            option = none;
            CHECK(option.is_empty());
            CHECK(aliveCount == 0);

            option = Option::some_emplace(std::string{"data"}, callbacks);
            option = Option::none();
            CHECK(option.is_empty());
            CHECK(aliveCount == 0);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Assigning some over none constructs a value") {
        {
            Option option = Option::none();
            const Option some = Option::some_emplace(std::string{"data"}, callbacks);
            CHECK(aliveCount == 1);

            option = some;
            CHECK(option.has_value());
            CHECK(aliveCount == 2);
        }
        CHECK(aliveCount == 0);

        {
            Option option = Option::none();
            // The temporary on the right is alive until the end of this statement, after which only option's new
            // value remains
            option = Option::some_emplace(std::string{"data"}, callbacks);
            CHECK(option.has_value());
            CHECK(aliveCount == 1);
        }
        CHECK(aliveCount == 0);
    }
}

// none() must never construct a value, so it has to work for a value type that can't be default constructed. This is
// a runtime test rather than a static_assert because only calling none() for real instantiates its body
TEST_CASE("Option of a type that isn't default constructible") {
    using NotDefaultConstructibleOption = pican::Option<NotDefaultConstructible>;

    const NotDefaultConstructibleOption none = NotDefaultConstructibleOption::none();
    CHECK(none.is_empty());

    const NotDefaultConstructibleOption some = NotDefaultConstructibleOption::some_emplace(42);
    CHECK(some.has_value());
    CHECK(some.value_or_panic().value == 42);
}
