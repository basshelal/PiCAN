module;

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <utility>

// Map the build system definition to our constexpr level
#ifndef CONTRACTS_ENABLED
#define CONTRACTS_ENABLED 1
#endif

export module contracts:Contracts;

import stacktrace;

export namespace contracts {
enum class ContractsLevel : std::uint8_t {
    NONE = 0,
    ALL = 1,
};

namespace _ {
void
_contract_violation_at_compile_time() {
    // No body because only called from a constexpr compile time so will always fail
}
}  // namespace _
}  // namespace contracts

namespace {
constexpr contracts::ContractsLevel level =
    (CONTRACTS_ENABLED != 0) ? contracts::ContractsLevel::ALL : contracts::ContractsLevel::NONE;

}  // namespace

export namespace contracts {
using ViolationHandler = void (*)(std::string_view msg);

inline void
default_violation_handler(std::string_view msg) {
    stacktrace::print_stacktrace(stderr, 2);
    if (!msg.empty()) {
        std::fprintf(stderr, "Contract violation: %.*s\n", static_cast<int>(msg.size()), msg.data());
    } else {
        std::fprintf(stderr, "Contract violation!\n");
    }
    std::abort();
}

inline ViolationHandler current_violation_handler = default_violation_handler;

inline void
set_violation_handler(ViolationHandler handler) {
    current_violation_handler = handler;
}

constexpr void
report_violation(std::string_view msg) {
    if consteval {
        _::_contract_violation_at_compile_time();
    } else {
        if (current_violation_handler) {
            current_violation_handler(msg);
        } else {
            default_violation_handler(msg);
        }
    }
}

constexpr void
assertion(bool cond, std::string_view msg = "") {
    if constexpr (level >= ContractsLevel::ALL) {
        if (!cond) {
            report_violation(msg);
        }
    }
}

constexpr void
precondition(bool cond, std::string_view msg = "") {
    if constexpr (level >= ContractsLevel::ALL) {
        if (!cond) {
            report_violation(msg);
        }
    }
}

template<typename Callable>
class PostconditionChecker {
    Callable func_f;

public:
    constexpr explicit PostconditionChecker(Callable func) : func_f(std::move(func)) {
    }

    PostconditionChecker(const PostconditionChecker&) = delete;
    PostconditionChecker(PostconditionChecker&&) = delete;
    PostconditionChecker&
    operator=(const PostconditionChecker&) = delete;
    PostconditionChecker&
    operator=(PostconditionChecker&&) = delete;

    constexpr ~PostconditionChecker() noexcept(false) {
        if constexpr (level >= ContractsLevel::ALL) {
            this->func_f();
        }
    }
};

template<typename Callable>
[[nodiscard]]
constexpr PostconditionChecker<Callable>
postcondition(Callable func) {
    return PostconditionChecker<Callable>(std::move(func));
}

template<typename T>
class InvariantChecker {
    static_assert(
        requires(const T& t) {
            { t.invariants() } -> std::same_as<void>;
        }, "Class must define a public `void invariants() const` method to use class invariants"
    );

    const T* obj_f;

public:
    constexpr explicit InvariantChecker(const T* obj) : obj_f(obj) {
        if constexpr (level >= ContractsLevel::ALL) {
            this->obj_f->check_invariants();
        }
    }

    InvariantChecker(const InvariantChecker&) = delete;
    InvariantChecker(InvariantChecker&&) = delete;
    InvariantChecker&
    operator=(const InvariantChecker&) = delete;
    InvariantChecker&
    operator=(InvariantChecker&&) = delete;

    constexpr ~InvariantChecker() noexcept(false) {
        if constexpr (level >= ContractsLevel::ALL) {
            this->obj_f->check_invariants();
        }
    }
};

template<typename T>
[[nodiscard]]
constexpr InvariantChecker<T>
check_invariants(const T* obj) {
    return InvariantChecker<T>(obj);
}

template<typename Callable>
class LoopInvariantChecker {
private:
    Callable func_f;

public:
    constexpr explicit LoopInvariantChecker(Callable func) : func_f(std::move(func)) {
        if constexpr (level >= ContractsLevel::ALL) {
            this->func_f();
        }
    }

    LoopInvariantChecker(const LoopInvariantChecker&) = delete;
    LoopInvariantChecker(LoopInvariantChecker&&) = delete;
    LoopInvariantChecker&
    operator=(const LoopInvariantChecker&) = delete;
    LoopInvariantChecker&
    operator=(LoopInvariantChecker&&) = delete;

    constexpr ~LoopInvariantChecker() noexcept {
        if constexpr (level >= ContractsLevel::ALL) {
            this->func_f();
        }
    }
};

template<typename Callable>
[[nodiscard]]
constexpr LoopInvariantChecker<Callable>
loop_invariant(Callable func) {
    return LoopInvariantChecker<Callable>(std::move(func));
}

}  // namespace contracts
