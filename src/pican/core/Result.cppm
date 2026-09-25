module;

#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

export module pican.core:Result;

import :functions;

export namespace pican {
template<typename Success_TP, typename Failure_TP>
class Result {
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

public:  // copy-control
    Result(const Result& rhs) : isSuccess_f(rhs.isSuccess_f) {
        if (this->isSuccess_f) {
            new (&this->success_f) SuccessType(rhs.success_f);
        } else {
            new (&this->failure_f) FailureType(rhs.failure_f);
        }
    }

    Result(Result&& rhs) : isSuccess_f(rhs.isSuccess_f) {
        if (this->isSuccess_f) {
            new (&this->success_f) SuccessType(std::move(rhs.success_f));
        } else {
            new (&this->failure_f) FailureType(std::move(rhs.failure_f));
        }
    }

    Result&
    operator=(const Result& rhs) &
        requires(std::is_copy_assignable_v<Success_TP> && std::is_copy_assignable_v<Failure_TP>)
    {
        if (this == &rhs) {
            return *this;
        }
        this->isSuccess_f = rhs.isSuccess_f;
        if (this->isSuccess_f) {
            this->success_f = rhs.success_f;
        } else {
            this->failure_f = rhs.failure_f;
        }
        return *this;
    }

    Result&
    operator=(Result&& rhs) &
        requires(std::is_move_assignable_v<Success_TP> && std::is_move_assignable_v<Failure_TP>)
    {
        if (this == &rhs) {
            return *this;
        }
        this->isSuccess_f = rhs.isSuccess_f;
        if (this->isSuccess_f) {
            this->success_f = std::move(rhs.success_f);
        } else {
            this->failure_f = std::move(rhs.failure_f);
        }
        return *this;
    }

    ~Result() {
        if (this->isSuccess_f) {
            this->success_f.~SuccessType();
        } else {
            this->failure_f.~FailureType();
        }
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
    success_value_or_else(const SuccessType& defaultValue) && = delete;

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

    [[nodiscard]]
    const FailureType&
    failure_value_or_else(const FailureType& defaultValue) && = delete;

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

template<typename Failure_TP>
using SimpleResult = Result<nullptr_t, Failure_TP>;

}  // namespace pican
