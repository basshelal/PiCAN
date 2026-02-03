#pragma once

// CONTRACTS_ENABLED = 0: No contracts
// Otherwise: All contracts enabled
// This is done so that, if needed in the future, we can separate contract enabling into more granular levels
#ifndef CONTRACTS_ENABLED
#define CONTRACTS_ENABLED 1
#endif

#if CONTRACTS_ENABLED == 0

#include <functional>

#include "pican/Utils.hpp"

namespace pican::contracts {
using ViolationHandler = std::function<void(const char* expression, SourceLocation sourceLocation)>;

constexpr auto defaultViolationHandler_g = [](const char* expression, SourceLocation sourceLocation) -> void {
};

inline ViolationHandler violationHandler_g = pican::contracts::defaultViolationHandler_g;
}  // namespace pican::contracts

#define CONTRACTS_ASSERT(condition) PREPROCESSOR_BLOCK()

#define CONTRACTS_PRECONDITION(condition) PREPROCESSOR_BLOCK()

#define CONTRACTS_POSTCONDITION(condition) PREPROCESSOR_BLOCK()

#define CONTRACTS_DEFINE_OLD_VAR(variable) PREPROCESSOR_BLOCK()

#define CONTRACTS_GET_OLD_VAR(variable) PREPROCESSOR_BLOCK()

#define CONTRACTS_CREATE_CODE_BLOCK(...) PREPROCESSOR_BLOCK()

#define CONTRACTS_DEFINE_CLASS_INVARIANTS(...)

#define CONTRACTS_ASSERT_CLASS_INVARIANTS() PREPROCESSOR_BLOCK()

#endif


#if CONTRACTS_ENABLED != 0

#include <functional>

#include <fmt/format.h>

#include "pican/SourceLocation.hpp"
#include "pican/Utils.hpp"

namespace pican::contracts {

using ViolationHandler = std::function<void(const char* expression, SourceLocation sourceLocation)>;

constexpr auto defaultViolationHandler_g = [](const char* expression, SourceLocation sourceLocation) -> void {
    pican::panic(fmt::format(fmt::runtime("Contract violated: {} at {}"), expression, sourceLocation.format()));
};

inline ViolationHandler violationHandler_g = pican::contracts::defaultViolationHandler_g;

template<typename Function_TP>
class PostCondition {
private:  // fields
    Function_TP function_f;
    const char* condition_f;
    SourceLocation sourceLocation_f;

public:  // constructors
    PostCondition(const Function_TP& function, const char* condition, const SourceLocation& sourceLocation) :
        function_f{function}, condition_f{condition}, sourceLocation_f{sourceLocation} {
    }

public:  // copy-control
    PostCondition(const PostCondition& rhs) = delete;

    PostCondition(PostCondition&& rhs) noexcept = delete;

    PostCondition&
    operator=(const PostCondition& rhs) & = delete;

    PostCondition&
    operator=(PostCondition&& rhs) & noexcept = delete;

    ~PostCondition() noexcept {
        if (!this->function_f()) {
            pican::contracts::violationHandler_g(this->condition_f, this->sourceLocation_f);
        }
    }
};

template<typename Lambda_TP>
class InvariantChecker {
private:  // fields
    const Lambda_TP lambda_f;
    const char* functionName_f;

public:  // functions
    explicit InvariantChecker(const Lambda_TP& lambda, const char* functionName) :
        lambda_f{lambda}, functionName_f{functionName} {
        this->lambda_f();
    }

    ~InvariantChecker() noexcept {
        this->lambda_f();
    }

public:  // copy-control
    InvariantChecker(const InvariantChecker& rhs) = delete;

    InvariantChecker(InvariantChecker&& rhs) noexcept = delete;

    InvariantChecker&
    operator=(const InvariantChecker& rhs) & = delete;

    InvariantChecker&
    operator=(InvariantChecker&& rhs) & noexcept = delete;
};
}  // namespace pican::contracts

#define CONTRACTS_ASSERT(condition) \
    PREPROCESSOR_BLOCK(if (!(condition)) { pican::contracts::violationHandler_g(#condition, CURRENT_SOURCE_LOCATION); })

#define CONTRACTS_PRECONDITION(condition) CONTRACTS_ASSERT(condition)

#define CONTRACTS_POSTCONDITION(condition)                  \
    const pican::contracts::PostCondition post_##__LINE__ { \
        [&]() {                                             \
            return (condition);                             \
        },                                                  \
            #condition, CURRENT_SOURCE_LOCATION             \
    }

#define CONTRACTS_DEFINE_OLD_VAR(variable) const auto variable_old = variable

#define CONTRACTS_GET_OLD_VAR(variable) variable_old

#define CONTRACTS_CREATE_CODE_BLOCK(...) PREPROCESSOR_BLOCK(__VA_ARGS__)

#define CONTRACTS_DEFINE_CLASS_INVARIANTS(...) \
private:                                       \
    void _class_invariants_block() const {     \
        __VA_ARGS__                            \
    }

#define CONTRACTS_ASSERT_CLASS_INVARIANTS()                 \
    pican::contracts::InvariantChecker _invariant_checker { \
        [this]() -> void {                                  \
            this->_class_invariants_block();                \
        },                                                  \
            __FUNCTION__                                    \
    }

#endif
