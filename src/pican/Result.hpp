#pragma once

#include <algorithm>
#include <utility>

#include "pican/Utils.hpp"

namespace pican {
template<typename Success_TP, typename Failure_TP>
class Result {
private:  // types
    struct SuccessConstructorParameter {};

    struct FailureConstructorParameter {};

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
    explicit Result(const SuccessType& s, SuccessConstructorParameter _trick) : isSuccess_f(true), success_f(s) {
    }

    // success_by_move
    explicit Result(SuccessType&& s, SuccessConstructorParameter _trick) :
        isSuccess_f(true), success_f(std::forward<SuccessType>(s)) {
    }

    // success_emplace
    template<typename... Args_TP>
    explicit Result(SuccessConstructorParameter _trick, Args_TP... args) :
        isSuccess_f(true), success_f(SuccessType(args...)) {
    }

    // failure_by_copy
    explicit Result(const FailureType& f, FailureConstructorParameter _trick) : isSuccess_f(false), failure_f(f) {
    }

    // failure_by_move
    explicit Result(FailureType&& f, FailureConstructorParameter _trick) :
        isSuccess_f(false), failure_f(std::forward<FailureType>(f)) {
    }

    // failure_emplace
    template<typename... Args_TP>
    explicit Result(FailureConstructorParameter _trick, Args_TP... args) :
        isSuccess_f(false), failure_f(FailureType(args...)) {
    }

public:  // copy-control
    Result(const Result& rhs) = default;

    Result(Result&& rhs) noexcept = default;

    Result&
    operator=(const Result& rhs) & = delete;

    Result&
    operator=(Result&& rhs) & noexcept = delete;

    ~Result() {
        if (this->isSuccess_f) {
            this->success_f.~SuccessType();
        } else {
            this->failure_f.~FailureType();
        }
    };

public:  // factory functions
    static Result
    success_by_copy(const SuccessType& s) {
        return Result{s, SuccessConstructorParameter{}};
    }

    static Result
    success_by_move(SuccessType&& s) {
        return Result{std::forward<SuccessType>(s), SuccessConstructorParameter{}};
    }

    template<typename... Args_TP>
    [[nodiscard]]
    static Result
    success_emplace(Args_TP&&... args) {
        return Result{SuccessConstructorParameter{}, args...};
    }

    [[nodiscard]]
    static Result
    success_default() {
        return Result{SuccessConstructorParameter{}};
    }

    [[nodiscard]]
    static Result
    failure_by_copy(const FailureType& f) {
        return Result{f, FailureConstructorParameter{}};
    }

    [[nodiscard]]
    static Result
    failure_by_move(FailureType&& f) {
        return Result{std::forward<FailureType>(f), FailureConstructorParameter{}};
    }

    template<typename... Args_TP>
    [[nodiscard]]
    static Result
    failure_emplace(Args_TP&&... args) {
        return Result{FailureConstructorParameter(), args...};
    }

    [[nodiscard]]
    static Result
    failure_default() {
        return Result{FailureConstructorParameter()};
    }

public:  // member functions
    [[nodiscard]]
    bool
    is_success() const {
        return this->isSuccess_f;
    }

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

    [[nodiscard]]
    const SuccessType&
    success_value_or_else(const SuccessType& defaultValue) && = delete;

    [[nodiscard]]
    bool
    is_failure() const {
        return !this->isSuccess_f;
    }

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

    [[nodiscard]]
    const FailureType&
    failure_value_or_else(const FailureType& defaultValue) && = delete;
};

template<typename Failure_TP>
using SimpleResult = Result<nullptr_t, Failure_TP>;

}  // namespace pican
