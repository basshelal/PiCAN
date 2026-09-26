#include <string>
#include <utility>

#include <catch2/catch_all.hpp>

#include "pican/core/core.hpp"
#include "test/utils/test_utils.hpp"

using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;

// TODO 26-Sep-26 12:22 The types should be different actually! Look into Catch typed tests to run the tests
//  using some different types with different sizes (int, uint64_t, etc etc)
using SuccessType = Tracked<std::string>;
using FailureType = Tracked<std::string>;
using Result = pican::Result<SuccessType, FailureType>;

namespace {

enum class CreationMethod : std::uint8_t {
    BY_COPY,
    BY_MOVE,
    EMPLACE,
    DEFAULT,
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
    }

    return "Unknown";
}

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
    }
    pican::panic("Unreachable");
}

Result
create_result(CreationMethod method, bool isSuccess, const std::string& data) {
    switch (method) {
        case CreationMethod::BY_COPY: {
            if (isSuccess) {
                SuccessType s{data};
                return Result::success_by_copy(s);
            }
            FailureType f{data};
            return Result::failure_by_copy(f);
        }
        case CreationMethod::BY_MOVE: {
            if (isSuccess) {
                SuccessType s{data};
                return Result::success_by_move(std::move(s));
            }
            FailureType f{data};
            return Result::failure_by_move(std::move(f));
        }
        case CreationMethod::EMPLACE: {
            if (isSuccess) {
                return Result::success_emplace(data);
            }
            return Result::failure_emplace(data);
        }
        case CreationMethod::DEFAULT: {
            if (isSuccess) {
                return Result::success_default();
            }
            return Result::failure_default();
        }
    }
    pican::panic("Unreachable");
}
}  // namespace

TEST_CASE("Result") {
    const bool isSuccess = GENERATE(true, false);
    const bool isFailure = !isSuccess;
    const CreationMethod creationMethod =
        GENERATE(CreationMethod::BY_COPY, CreationMethod::BY_MOVE, CreationMethod::EMPLACE, CreationMethod::DEFAULT);

    DYNAMIC_SECTION("Creation: " << to_string(creationMethod) << (isSuccess ? " Success" : " Failure")) {
        const std::string data = isSuccess ? "success_data" : "failure_data";
        const std::string defaultData = isSuccess ? "default_success_data" : "default_failure_data";
        std::string expectedData;
        if (creationMethod == CreationMethod::DEFAULT) {
            expectedData = std::string{};
        } else {
            expectedData = data;
        }

        // non const to allow for mutable refs pointing to these
        SuccessType defaultSuccessType{defaultData};
        FailureType defaultFailureType{defaultData};

        Result resultMut = create_result(creationMethod, isSuccess, data);
        CHECK(resultMut.is_success() == isSuccess);
        CHECK(resultMut.is_failure() == isFailure);

        const Result resultConst = create_result(creationMethod, isSuccess, data);
        CHECK(resultConst.is_success() == isSuccess);
        CHECK(resultConst.is_failure() == isFailure);

        SECTION("success_value_or_else") {
            SuccessType& defaultRef = defaultSuccessType;
            SuccessType& ref = resultMut.success_value_or_else(defaultRef);
            if (isSuccess) {
                CHECK(ref.data == expectedData);
                CHECK_FALSE(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(check_matches(creationMethod, ref.lastOperation, false));
            } else {
                CHECK(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(ref.data == defaultData);
                CHECK(ref.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }

            const SuccessType& defaultRefConst = defaultSuccessType;
            const SuccessType& refConst = resultConst.success_value_or_else(defaultRefConst);
            if (isSuccess) {
                CHECK(refConst.data == expectedData);
                CHECK_FALSE(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            } else {
                CHECK(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(refConst.data == defaultData);
                CHECK(refConst.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }
        }

        SECTION("failure_value_or_else") {
            FailureType& defaultRef = defaultFailureType;
            FailureType& ref = resultMut.failure_value_or_else(defaultRef);
            if (isFailure) {
                CHECK(ref.data == expectedData);
                CHECK_FALSE(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(check_matches(creationMethod, ref.lastOperation, false));
            } else {
                CHECK(std::addressof(ref) == std::addressof(defaultRef));
                CHECK(ref.data == defaultData);
                CHECK(ref.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }

            const FailureType& defaultRefConst = defaultFailureType;
            const FailureType& refConst = resultConst.failure_value_or_else(defaultRefConst);
            if (isFailure) {
                CHECK(refConst.data == expectedData);
                CHECK_FALSE(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            } else {
                CHECK(std::addressof(refConst) == std::addressof(defaultRefConst));
                CHECK(refConst.data == defaultData);
                CHECK(refConst.lastOperation == LifetimeOperation::CONSTRUCTOR);
            }
        }

        if (isSuccess) {
            SECTION("success_value_or_panic") {
                SuccessType& ref = resultMut.success_value_or_panic();
                CHECK(ref.data == expectedData);
                CHECK(check_matches(creationMethod, ref.lastOperation, false));

                const SuccessType& refConst = resultConst.success_value_or_panic();
                CHECK(refConst.data == expectedData);
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            }

            SECTION("success_value_extract_or_panic") {
                const SuccessType extracted = std::move(resultMut).success_value_extract_or_panic();
                CHECK(extracted.data == expectedData);
                CHECK(extracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                // Extracting moves the value out but doesn't change the Result's state, it's still a success holding a
                // moved-from value, reading is_success() after the move is deliberate
                CHECK(resultMut.is_success());
            }
        }

        if (isFailure) {
            SECTION("failure_value_or_panic") {
                FailureType& ref = resultMut.failure_value_or_panic();
                CHECK(ref.data == expectedData);
                CHECK(check_matches(creationMethod, ref.lastOperation, false));

                const FailureType& refConst = resultConst.failure_value_or_panic();
                CHECK(refConst.data == expectedData);
                CHECK(check_matches(creationMethod, refConst.lastOperation, false));
            }

            SECTION("failure_value_extract_or_panic") {
                const FailureType extracted = std::move(resultMut).failure_value_extract_or_panic();
                CHECK(extracted.data == expectedData);
                CHECK(extracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                // See success_value_extract_or_panic above
                CHECK(resultMut.is_failure());
            }
        }

        SECTION("Copy Constructor") {
            Result copy{resultMut};
            CHECK(copy.is_success() == isSuccess);

            if (isSuccess) {
                CHECK(copy.success_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                CHECK(copy.success_value_or_panic().data == expectedData);
            } else {
                CHECK(copy.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                CHECK(copy.failure_value_or_panic().data == expectedData);
            }
        }

        SECTION("Move Constructor") {
            Result moved{std::move(resultMut)};
            CHECK(moved.is_success() == isSuccess);

            if (isSuccess) {
                CHECK(moved.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                CHECK(moved.success_value_or_panic().data == expectedData);
            } else {
                CHECK(moved.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
                CHECK(moved.failure_value_or_panic().data == expectedData);
            }
        }

        SECTION("Self Assignment") {
            // Through references so the compiler doesn't warn about the obvious self assignment, the point is to hit
            // the this == &rhs early return, so nothing should change, not even lastOperation
            const Result& selfConst = resultMut;
            resultMut = selfConst;
            CHECK(resultMut.is_success() == isSuccess);
            if (isSuccess) {
                CHECK(resultMut.success_value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, resultMut.success_value_or_panic().lastOperation, false));
            } else {
                CHECK(resultMut.failure_value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, resultMut.failure_value_or_panic().lastOperation, false));
            }

            Result& selfMut = resultMut;
            resultMut = std::move(selfMut);
            CHECK(resultMut.is_success() == isSuccess);
            if (isSuccess) {
                CHECK(resultMut.success_value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, resultMut.success_value_or_panic().lastOperation, false));
            } else {
                CHECK(resultMut.failure_value_or_panic().data == expectedData);
                CHECK(check_matches(creationMethod, resultMut.failure_value_or_panic().lastOperation, false));
            }
        }

        const bool assignFromSuccess = GENERATE(true, false);
        DYNAMIC_SECTION("Assignment from: " << (assignFromSuccess ? "Success" : "Failure")) {
            const std::string assignData = assignFromSuccess ? "assign_success_data" : "assign_failure_data";
            Result assignSource = create_result(CreationMethod::EMPLACE, assignFromSuccess, assignData);
            const bool isSameAlternative = isSuccess == assignFromSuccess;

            SECTION("Copy Assignment") {
                resultMut = assignSource;
                const LifetimeOperation expectedOperation =
                    isSameAlternative ? LifetimeOperation::COPY_ASSIGNMENT : LifetimeOperation::COPY_CONSTRUCTOR;

                CHECK(resultMut.is_success() == assignFromSuccess);
                if (assignFromSuccess) {
                    const SuccessType& successRefConst = resultMut.success_value_or_panic();
                    CHECK(successRefConst.data == assignData);
                    CHECK(successRefConst.lastOperation == expectedOperation);
                    // A copy must leave its source untouched
                    CHECK(assignSource.success_value_or_panic().data == assignData);
                } else {
                    const FailureType& failureRefConst = resultMut.failure_value_or_panic();
                    CHECK(failureRefConst.data == assignData);
                    CHECK(failureRefConst.lastOperation == expectedOperation);
                    // A copy must leave its source untouched
                    CHECK(assignSource.failure_value_or_panic().data == assignData);
                }
            }

            SECTION("Move Assignment") {
                resultMut = std::move(assignSource);
                const LifetimeOperation expectedOperation =
                    isSameAlternative ? LifetimeOperation::MOVE_ASSIGNMENT : LifetimeOperation::MOVE_CONSTRUCTOR;

                CHECK(resultMut.is_success() == assignFromSuccess);
                if (assignFromSuccess) {
                    const SuccessType& successRefConst = resultMut.success_value_or_panic();
                    CHECK(successRefConst.data == assignData);
                    CHECK(successRefConst.lastOperation == expectedOperation);
                } else {
                    const FailureType& failureRefConst = resultMut.failure_value_or_panic();
                    CHECK(failureRefConst.data == assignData);
                    CHECK(failureRefConst.lastOperation == expectedOperation);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Generated by Claude
////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks that Result starts and ends the lifetime of its values exactly once each. aliveCount goes up whenever a
// Tracked is constructed (in any way) and down whenever one is destroyed, so if Result ever forgets to destroy a value,
// destroys one twice, or destroys one it never constructed, the count ends up wrong.
// Note that while SuccessType and FailureType are the same type (see the TODO at the top), the old cross-alternative
// assignment bug is invisible here, because assigning into the dead member lands on a live object of the same type.
// It is caught by the "Assignment from" sections above through lastOperation, and would be caught here too once the
// types differ
TEST_CASE("Result lifetimes are balanced") {
    int aliveCount = 0;
    // Generic lambdas so they convert to both SuccessType's and FailureType's LifetimeCallback, even once those differ
    const auto onStarted = [&aliveCount](const auto&) {
        aliveCount++;
    };
    const auto onEnded = [&aliveCount](const auto&) {
        aliveCount--;
    };
    const SuccessType::LifetimeCallbacks successCallbacks{
        .onConstructed = onStarted,
        .onCopyConstructed = onStarted,
        .onMoveConstructed = onStarted,
        .onDestructed = onEnded,
    };
    const FailureType::LifetimeCallbacks failureCallbacks{
        .onConstructed = onStarted,
        .onCopyConstructed = onStarted,
        .onMoveConstructed = onStarted,
        .onDestructed = onEnded,
    };

    SECTION("Success is destroyed with its Result") {
        {
            const Result result = Result::success_emplace(std::string{"success_data"}, successCallbacks);
            CHECK(aliveCount == 1);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Failure is destroyed with its Result") {
        {
            const Result result = Result::failure_emplace(std::string{"failure_data"}, failureCallbacks);
            CHECK(aliveCount == 1);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Copies and moves are all destroyed") {
        {
            Result original = Result::success_emplace(std::string{"success_data"}, successCallbacks);
            const Result copy{original};
            CHECK(aliveCount == 2);
            // The moved-from original still holds a (moved-from) value, so it still counts as alive
            const Result moved{std::move(original)};
            CHECK(aliveCount == 3);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Assigning across alternatives") {
        {
            Result result = Result::success_emplace(std::string{"success_data"}, successCallbacks);
            const Result failure = Result::failure_emplace(std::string{"failure_data"}, failureCallbacks);
            CHECK(aliveCount == 2);

            // Our success is destroyed and a copy of the failure constructed, so the count doesn't change
            result = failure;
            CHECK(result.is_failure());
            CHECK(aliveCount == 2);

            // The temporary on the right is alive until the end of this statement, after which only result's new
            // success and failure remain
            result = Result::success_emplace(std::string{"success_data"}, successCallbacks);
            CHECK(result.is_success());
            CHECK(aliveCount == 2);
        }
        CHECK(aliveCount == 0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// End generated by Claude
////////////////////////////////////////////////////////////////////////////////////////////////////
