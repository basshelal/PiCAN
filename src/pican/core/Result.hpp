#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include "pican/core/concepts.hpp"
#include "pican/core/functions.hpp"

namespace pican {
template<typename Success_TP, typename Failure_TP>
class Result {
    static_assert(
        std::is_object_v<Success_TP>, "Result success type must be an object type (not a reference, void or function)"
    );
    static_assert(!std::is_array_v<Success_TP>, "Result success type must not be an array");
    static_assert(
        std::is_object_v<Failure_TP>, "Result failure type must be an object type (not a reference, void or function)"
    );
    static_assert(!std::is_array_v<Failure_TP>, "Result failure type must not be an array");

private:  // types
    struct SuccessParameterTag {};

    struct FailureParameterTag {};

public:  // types
    using SuccessType = Success_TP;
    using FailureType = Failure_TP;

private:  // member fields
    bool isSuccess_f;

    union {
        SuccessType success_f;
        FailureType failure_f;
    };

private:  // constructors
    // success_by_copy
    explicit Result(const SuccessType& s, SuccessParameterTag tag) : isSuccess_f(true), success_f(s) {
    }

    // success_by_move
    explicit Result(SuccessType&& s, SuccessParameterTag tag) :
        isSuccess_f(true), success_f(std::forward<SuccessType>(s)) {
    }

    // success_emplace
    template<typename... Args_TP>
    explicit Result(SuccessParameterTag tag, Args_TP&&... args) :
        isSuccess_f(true), success_f(std::forward<Args_TP>(args)...) {
    }

    // failure_by_copy
    explicit Result(const FailureType& f, FailureParameterTag tag) : isSuccess_f(false), failure_f(f) {
    }

    // failure_by_move
    explicit Result(FailureType&& f, FailureParameterTag tag) :
        isSuccess_f(false), failure_f(std::forward<FailureType>(f)) {
    }

    // failure_emplace
    template<typename... Args_TP>
    explicit Result(FailureParameterTag tag, Args_TP&&... args) :
        isSuccess_f(false), failure_f(std::forward<Args_TP>(args)...) {
    }

private:  // lifetime helpers
    // Constructs, by copy, whichever alternative is alive in rhs into this. This must have no alive alternative when
    // called (either freshly constructed or just destroyed), and the caller is responsible for isSuccess_f
    void
    construct_alive_from(const Result& rhs) {
        if (rhs.isSuccess_f) {
            std::construct_at(std::addressof(this->success_f), rhs.success_f);
        } else {
            std::construct_at(std::addressof(this->failure_f), rhs.failure_f);
        }
    }

    // Same as above but by move, rhs keeps its alternative alive but in a moved-from state, like std::optional does
    void
    construct_alive_from(Result&& rhs) {
        if (rhs.isSuccess_f) {
            std::construct_at(std::addressof(this->success_f), std::move(rhs.success_f));
        } else {
            std::construct_at(std::addressof(this->failure_f), std::move(rhs.failure_f));
        }
    }

    // Ends the lifetime of whichever alternative is alive, after this no alternative is alive until one of the
    // construct_alive_from functions is called
    void
    destroy_alive() {
        if (this->isSuccess_f) {
            std::destroy_at(std::addressof(this->success_f));
        } else {
            std::destroy_at(std::addressof(this->failure_f));
        }
    }

public:  // copy-control
    Result(const Result& rhs)
        requires AllCopyableOnly<SuccessType, FailureType>
        : isSuccess_f(rhs.isSuccess_f) {
        this->construct_alive_from(rhs);
    }

    Result(
        Result&& rhs
    ) noexcept(std::is_nothrow_move_constructible_v<Success_TP> && std::is_nothrow_move_constructible_v<Failure_TP>)
        requires(std::is_move_constructible_v<Success_TP> && std::is_move_constructible_v<Failure_TP>)
        : isSuccess_f(rhs.isSuccess_f) {
        this->construct_alive_from(std::move(rhs));
    }

    Result&
    operator=(const Result& rhs) &
        requires AllCopyableOnly<SuccessType, FailureType>
    {
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if (this->isSuccess_f == rhs.isSuccess_f) {
            if (this->isSuccess_f) {
                this->success_f = rhs.success_f;
            } else {
                this->failure_f = rhs.failure_f;
            }
        } else {
            // Different alternatives, the member we would assign to is not alive, so end the lifetime of ours and
            // start the lifetime of theirs, only then does the flag change so it always describes what is alive
            this->destroy_alive();
            this->construct_alive_from(rhs);
            this->isSuccess_f = rhs.isSuccess_f;
        }
        return *this;
    }

    Result&
    operator=(Result&& rhs) & noexcept(
        std::is_nothrow_move_constructible_v<Success_TP> && std::is_nothrow_move_constructible_v<Failure_TP> &&
        std::is_nothrow_move_assignable_v<Success_TP> && std::is_nothrow_move_assignable_v<Failure_TP>
    )
        requires(
            std::is_move_constructible_v<Success_TP> && std::is_move_constructible_v<Failure_TP> &&
            std::is_move_assignable_v<Success_TP> && std::is_move_assignable_v<Failure_TP>
        )
    {
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if (this->isSuccess_f == rhs.isSuccess_f) {
            // Same alternative is alive on both sides, so there is a live object on our side to assign to
            if (this->isSuccess_f) {
                this->success_f = std::move(rhs.success_f);
            } else {
                this->failure_f = std::move(rhs.failure_f);
            }
        } else {
            // Different alternatives, see the copy assignment operator above
            this->destroy_alive();
            this->construct_alive_from(std::move(rhs));
            this->isSuccess_f = rhs.isSuccess_f;
        }
        return *this;
    }

    ~Result() {
        this->destroy_alive();
    }

public:  // factory functions
    [[nodiscard]]
    static Result
    success_by_copy(const SuccessType& s) {
        return Result{s, SuccessParameterTag{}};
    }

    [[nodiscard]]
    static Result
    success_by_move(SuccessType&& s) {
        return Result{std::forward<SuccessType>(s), SuccessParameterTag{}};
    }

    template<typename... Args_TP>
    [[nodiscard]]
    static Result
    success_emplace(Args_TP&&... args) {
        return Result{SuccessParameterTag{}, std::forward<Args_TP>(args)...};
    }

    [[nodiscard]]
    static Result
    success_default() {
        return Result{SuccessParameterTag{}};
    }

    [[nodiscard]]
    static Result
    failure_by_copy(const FailureType& f) {
        return Result{f, FailureParameterTag{}};
    }

    [[nodiscard]]
    static Result
    failure_by_move(FailureType&& f) {
        return Result{std::forward<FailureType>(f), FailureParameterTag{}};
    }

    template<typename... Args_TP>
    [[nodiscard]]
    static Result
    failure_emplace(Args_TP&&... args) {
        return Result{FailureParameterTag{}, std::forward<Args_TP>(args)...};
    }

    [[nodiscard]]
    static Result
    failure_default() {
        return Result{FailureParameterTag{}};
    }

public:  // member functions
    [[nodiscard]]
    bool
    is_success() const {
        return this->isSuccess_f;
    }

    [[nodiscard]]
    const SuccessType&
    success_value_or_else(const SuccessType& defaultValue) const& {
        if (this->isSuccess_f) {
            return this->success_f;
        } else {
            return defaultValue;
        }
    }

    [[nodiscard]]
    SuccessType&
    success_value_or_else(SuccessType& defaultValue) & {
        if (this->isSuccess_f) {
            return this->success_f;
        } else {
            return defaultValue;
        }
    }

    const SuccessType&
    success_value_or_else(const SuccessType&& defaultValue) const& = delete;

    SuccessType&
    success_value_or_else(SuccessType&& defaultValue) & = delete;

    const SuccessType&
    success_value_or_else(const SuccessType& defaultValue) const&& = delete;

    [[nodiscard]]
    const SuccessType&
    success_value_or_panic() const& {
        if (this->isSuccess_f) {
            return this->success_f;
        }
        pican::panic("Result was failure, expected success");
    }

    [[nodiscard]]
    SuccessType&
    success_value_or_panic() & {
        if (this->isSuccess_f) {
            return this->success_f;
        }
        pican::panic("Result was failure, expected success");
    }

    [[nodiscard]]
    const SuccessType&
    success_value_or_panic() const&& = delete;

    [[nodiscard]]
    SuccessType&&
    success_value_extract_or_panic() && {
        if (this->isSuccess_f) {
            return std::move(this->success_f);
        }
        pican::panic("Result was failure, expected success");
    }

    [[nodiscard]]
    bool
    is_failure() const {
        return !this->isSuccess_f;
    }

    [[nodiscard]]
    const FailureType&
    failure_value_or_else(const FailureType& defaultValue) const& {
        if (!this->isSuccess_f) {
            return this->failure_f;
        } else {
            return defaultValue;
        }
    }

    [[nodiscard]]
    FailureType&
    failure_value_or_else(FailureType& defaultValue) & {
        if (!this->isSuccess_f) {
            return this->failure_f;
        } else {
            return defaultValue;
        }
    }

    const FailureType&
    failure_value_or_else(const FailureType&& defaultValue) const& = delete;

    FailureType&
    failure_value_or_else(FailureType&& defaultValue) & = delete;

    const FailureType&
    failure_value_or_else(const FailureType& defaultValue) const&& = delete;

    [[nodiscard]]
    const FailureType&
    failure_value_or_panic() const& {
        if (!this->isSuccess_f) {
            return this->failure_f;
        }
        pican::panic("Result was success, expected failure");
    }

    [[nodiscard]]
    FailureType&
    failure_value_or_panic() & {
        if (!this->isSuccess_f) {
            return this->failure_f;
        }
        pican::panic("Result was success, expected failure");
    }

    [[nodiscard]]
    const FailureType&
    failure_value_or_panic() const&& = delete;

    [[nodiscard]]
    FailureType&&
    failure_value_extract_or_panic() && {
        if (!this->isSuccess_f) {
            return std::move(this->failure_f);
        }
        pican::panic("Result was success, expected failure");
    }
};

}  // namespace pican
