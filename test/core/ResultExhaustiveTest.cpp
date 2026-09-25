#include <string>
#include <utility>

#include <catch2/catch_all.hpp>

import pican.test_utils;
import pican.core;

using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;

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
to_string(CreationMethod m) {
    switch (m) {
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
                FAIL("Impossible state" << to_string(method) << "  and isAssignment");
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

static Result
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

TEST_CASE("Result - Exhaustive Suite") {
    const bool isSuccess = GENERATE(true, false);
    const bool isFailure = !isSuccess;
    const CreationMethod creationMethod =
        GENERATE(CreationMethod::BY_COPY, CreationMethod::BY_MOVE, CreationMethod::EMPLACE, CreationMethod::DEFAULT);

    DYNAMIC_SECTION("Creation Method: " << to_string(creationMethod) << (isSuccess ? " Success" : " Failure")) {
        const std::string data = isSuccess ? "success_data" : "failure_data";
        const std::string defaultData = "default_data";
        std::string expectedData;
        if (creationMethod == CreationMethod::DEFAULT) {
            expectedData = std::string{};
        } else {
            expectedData = data;
        }

        SuccessType defaultSuccessType{defaultData};
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
            // TODO 25-Sept-26 17:45 Make it match like above
            if (isFailure) {
                FailureType& failureRef = resultMut.failure_value_or_panic();
                CHECK(failureRef.data == expectedData);
                CHECK(check_matches(creationMethod, failureRef.lastOperation, false));

                const FailureType& failureRefConst = resultConst.failure_value_or_panic();
                CHECK(failureRefConst.data == expectedData);
                CHECK(check_matches(creationMethod, failureRefConst.lastOperation, false));

                FailureType failureExtracted = std::move(resultMut).failure_value_extract_or_panic();
                CHECK(failureExtracted.data == expectedData);
                CHECK(failureExtracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
            }
        }

        SECTION("Copy Construction") {
            // Test that the copy constructor safely replicates state
            Result copy = resultMut;
            CHECK(copy.is_success() == resultMut.is_success());

            if (isSuccess) {
                CHECK(copy.success_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                if (creationMethod != CreationMethod::DEFAULT) {
                    CHECK(copy.success_value_or_panic().data == data);
                }
            } else {
                CHECK(copy.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                if (creationMethod != CreationMethod::DEFAULT) {
                    CHECK(copy.failure_value_or_panic().data == data);
                }
            }
        }

        SECTION("Move Construction") {
            // Test that the move constructor safely shifts state without extraneous copies
            Result moved = std::move(resultMut);
            CHECK(moved.is_success() == isSuccess);

            if (isSuccess) {
                CHECK(moved.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
            } else {
                CHECK(moved.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
            }
        }

        // Generate a sub-matrix for Assignments: Assigning from both Success/Failure
        // to our current Result instance (which might also be Success/Failure).
        // This covers transitions like Success->Failure, Failure->Success, etc.
        auto assignFromSuccess = GENERATE(true, false);
        DYNAMIC_SECTION("Assignment from State: " << (assignFromSuccess ? "Success" : "Failure")) {
            std::string assignData = assignFromSuccess ? "assign_success_data" : "assign_failure_data";
            Result assignSource = create_result(CreationMethod::EMPLACE, assignFromSuccess, assignData);

            SECTION("Copy Assignment") {
                resultMut = assignSource;

                CHECK(resultMut.is_success() == assignSource.is_success());
                if (assignFromSuccess) {
                    CHECK(resultMut.success_value_or_panic().data == assignData);

                    // NOTE: If state changes between Success <-> Failure, a proper union manager
                    // should destroy the old state and placement-new the new state. If it directly
                    // invokes copy assignment across a state boundary, it causes Undefined Behavior.
                    // This assertion will test exactly how the assignment operator handles lifetimes.
                    CHECK(
                        (resultMut.success_value_or_panic().lastOperation == LifetimeOperation::COPY_ASSIGNMENT ||
                         resultMut.success_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR)
                    );
                } else {
                    CHECK(resultMut.failure_value_or_panic().data == assignData);

                    CHECK(
                        (resultMut.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_ASSIGNMENT ||
                         resultMut.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR)
                    );
                }
            }

            SECTION("Move Assignment") {
                resultMut = std::move(assignSource);

                CHECK(resultMut.is_success() == assignFromSuccess);
                if (assignFromSuccess) {
                    CHECK(
                        (resultMut.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_ASSIGNMENT ||
                         resultMut.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR)
                    );
                } else {
                    CHECK(
                        (resultMut.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_ASSIGNMENT ||
                         resultMut.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR)
                    );
                }
            }
        }
    }
}

TEST_CASE("SimpleResult - Basic Checks") {
    using SimpleRes = pican::SimpleResult<FailureType>;  // Result<nullptr_t, FailureType>

    SECTION("Construct as Failure") {
        FailureType f{"simple_fail"};
        auto res = SimpleRes::failure_by_copy(f);
        CHECK(res.is_failure());
        CHECK(res.failure_value_or_panic().data == "simple_fail");
    }
}
