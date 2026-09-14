// 
// Exhaustive test suite for Result.cppm
// 
// This suite leverages Catch2's GENERATE and DYNAMIC_SECTION to compactly cover
// the entire combinatorial space of states (Success/Failure) and creation methods
// (Copy, Move, Emplace, Default). It tests copy/move semantics, accessors, 
// and cross-state assignments.

#include <string>
#include <tuple>
#include <utility>
#include <optional>
#include <catch2/catch_all.hpp>

import pican.test_utils;
import pican.core;

using pican::Result;
using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;

using SuccessType = Tracked<std::string>;
using FailureType = Tracked<std::string>;
using ResultType = Result<SuccessType, FailureType>;

// Represents how a Result object is initially constructed
enum class CreationMethod {
    BY_COPY,
    BY_MOVE,
    EMPLACE,
    DEFAULT
};

// Helper: Converts CreationMethod enum to a human-readable string for Catch2 sections
static std::string method_to_string(CreationMethod m) {
    switch (m) {
        case CreationMethod::BY_COPY: return "By Copy";
        case CreationMethod::BY_MOVE: return "By Move";
        case CreationMethod::EMPLACE: return "Emplace";
        case CreationMethod::DEFAULT: return "Default";
    }
    return "Unknown";
}

// Helper: Constructs a ResultType in a specific state using a specific method.
static ResultType create_result(bool asSuccess, CreationMethod method, const std::string& data) {
    if (asSuccess) {
        switch (method) {
            case CreationMethod::BY_COPY: {
                SuccessType s{data};
                return ResultType::success_by_copy(s);
            }
            case CreationMethod::BY_MOVE: {
                SuccessType s{data};
                return ResultType::success_by_move(std::move(s));
            }
            case CreationMethod::EMPLACE:
                return ResultType::success_emplace(data);
            case CreationMethod::DEFAULT:
                return ResultType::success_default();
        }
    } else {
        switch (method) {
            case CreationMethod::BY_COPY: {
                FailureType f{data};
                return ResultType::failure_by_copy(f);
            }
            case CreationMethod::BY_MOVE: {
                FailureType f{data};
                return ResultType::failure_by_move(std::move(f));
            }
            case CreationMethod::EMPLACE:
                return ResultType::failure_emplace(data);
            case CreationMethod::DEFAULT:
                return ResultType::failure_default();
        }
    }
    pican::panic("Unreachable");
}

TEST_CASE("Result - Exhaustive Suite") {
    // GENERATE will run the entire enclosing TEST_CASE dynamically for every combination 
    // of `isSuccess` and `creationMethod`. This compresses 8 distinct test branches into 1 block.
    auto isSuccess = GENERATE(true, false);
    auto creationMethod = GENERATE(
        CreationMethod::BY_COPY, 
        CreationMethod::BY_MOVE, 
        CreationMethod::EMPLACE, 
        CreationMethod::DEFAULT
    );

    // Create a uniquely named section for each iteration of the GENERATE matrix
    DYNAMIC_SECTION("State: " << (isSuccess ? "Success" : "Failure") << " | Creation: " << method_to_string(creationMethod)) {
        
        std::string primaryData = isSuccess ? "success_data" : "failure_data";
        
        // We use std::optional here to safely construct our test subject, `result`, 
        // deferring its destruction appropriately while manipulating it below.
        std::optional<ResultType> resultOpt;
        resultOpt.emplace(create_result(isSuccess, creationMethod, primaryData));
        ResultType& result = *resultOpt;

        SECTION("State Queries (is_success / is_failure)") {
            // Verify that state inquiry reflects how it was constructed
            CHECK(result.is_success() == isSuccess);
            CHECK(result.is_failure() == !isSuccess);
        }
        
        SECTION("Value Accessors - success_value_or_panic") {
            if (isSuccess) {
                // Check L-value mutable reference accessor
                SuccessType& sRef = result.success_value_or_panic();
                if (creationMethod != CreationMethod::DEFAULT) CHECK(sRef.data == primaryData);
                
                // Check const L-value reference accessor
                const ResultType& constResult = result;
                const SuccessType& cRef = constResult.success_value_or_panic();
                if (creationMethod != CreationMethod::DEFAULT) CHECK(cRef.data == primaryData);
                
                // Check R-value extractor 
                // Extracts the value via move semantics and verifies it registered a move
                SuccessType extracted = std::move(result).success_value_extract_or_panic();
                if (creationMethod != CreationMethod::DEFAULT) CHECK(extracted.data == primaryData);
                CHECK(extracted.lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR);
            }
            // Note: If !isSuccess, calling this panics by design.
        }

        SECTION("Value Accessors - failure_value_or_panic") {
            if (!isSuccess) {
                // Check L-value mutable reference accessor
                FailureType& fRef = result.failure_value_or_panic();
                if (creationMethod != CreationMethod::DEFAULT) CHECK(fRef.data == primaryData);
                
                // Check const L-value reference accessor
                const ResultType& constResult = result;
                const FailureType& cRef = constResult.failure_value_or_panic();
                if (creationMethod != CreationMethod::DEFAULT) CHECK(cRef.data == primaryData);
            }
            // Note: If isSuccess, calling this panics by design.
        }
        
        SECTION("Value Accessors - success_value_or_else") {
            SuccessType fallbackMut{"fallback_success"};
            const SuccessType fallbackConst{"fallback_success"};

            // Test L-value accessor with fallback
            SuccessType& resMut = result.success_value_or_else(fallbackMut);
            if (isSuccess) {
                // If it was a success, it must return a reference to the active union member
                CHECK(&resMut == &result.success_value_or_panic());
            } else {
                // If it was a failure, it must fall back to the provided reference
                CHECK(&resMut == &fallbackMut);
            }

            // Test const L-value accessor with fallback
            const ResultType& constResult = result;
            const SuccessType& resConst = constResult.success_value_or_else(fallbackConst);
            if (isSuccess) {
                CHECK(&resConst == &constResult.success_value_or_panic());
            } else {
                CHECK(&resConst == &fallbackConst);
            }
        }

        SECTION("Value Accessors - failure_value_or_else") {
            FailureType fallbackMut{"fallback_failure"};
            const FailureType fallbackConst{"fallback_failure"};

            // Test L-value accessor with fallback
            FailureType& resMut = result.failure_value_or_else(fallbackMut);
            if (!isSuccess) {
                // If it was a failure, it must return a reference to the active union member
                CHECK(&resMut == &result.failure_value_or_panic());
            } else {
                // If it was a success, it must fall back to the provided reference
                CHECK(&resMut == &fallbackMut);
            }

            // Test const L-value accessor with fallback
            const ResultType& constResult = result;
            const FailureType& resConst = constResult.failure_value_or_else(fallbackConst);
            if (!isSuccess) {
                CHECK(&resConst == &constResult.failure_value_or_panic());
            } else {
                CHECK(&resConst == &fallbackConst);
            }
        }

        SECTION("Copy Construction") {
            // Test that the copy constructor safely replicates state
            ResultType copy = result;
            CHECK(copy.is_success() == result.is_success());
            
            if (isSuccess) {
                CHECK(copy.success_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                if (creationMethod != CreationMethod::DEFAULT) {
                    CHECK(copy.success_value_or_panic().data == primaryData);
                }
            } else {
                CHECK(copy.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR);
                if (creationMethod != CreationMethod::DEFAULT) {
                    CHECK(copy.failure_value_or_panic().data == primaryData);
                }
            }
        }

        SECTION("Move Construction") {
            // Test that the move constructor safely shifts state without extraneous copies
            ResultType moved = std::move(result);
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
            ResultType assignSource = create_result(assignFromSuccess, CreationMethod::EMPLACE, assignData);
            
            SECTION("Copy Assignment") {
                result = assignSource;
                
                CHECK(result.is_success() == assignSource.is_success());
                if (assignFromSuccess) {
                    CHECK(result.success_value_or_panic().data == assignData);
                    
                    // NOTE: If state changes between Success <-> Failure, a proper union manager 
                    // should destroy the old state and placement-new the new state. If it directly 
                    // invokes copy assignment across a state boundary, it causes Undefined Behavior.
                    // This assertion will test exactly how the assignment operator handles lifetimes.
                    CHECK((result.success_value_or_panic().lastOperation == LifetimeOperation::COPY_ASSIGNMENT ||
                           result.success_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR));
                } else {
                    CHECK(result.failure_value_or_panic().data == assignData);
                    
                    CHECK((result.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_ASSIGNMENT ||
                           result.failure_value_or_panic().lastOperation == LifetimeOperation::COPY_CONSTRUCTOR));
                }
            }
            
            SECTION("Move Assignment") {
                result = std::move(assignSource);
                
                CHECK(result.is_success() == assignFromSuccess);
                if (assignFromSuccess) {
                    CHECK((result.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_ASSIGNMENT ||
                           result.success_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR));
                } else {
                    CHECK((result.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_ASSIGNMENT ||
                           result.failure_value_or_panic().lastOperation == LifetimeOperation::MOVE_CONSTRUCTOR));
                }
            }
        }
    }
}

TEST_CASE("SimpleResult - Basic Checks") {
    using SimpleRes = pican::SimpleResult<FailureType>; // Result<nullptr_t, FailureType>
    
    SECTION("Construct as Failure") {
        FailureType f{"simple_fail"};
        auto res = SimpleRes::failure_by_copy(f);
        CHECK(res.is_failure());
        CHECK(res.failure_value_or_panic().data == "simple_fail");
    }
}
