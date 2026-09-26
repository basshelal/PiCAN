#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "pican/core/functions.hpp"

namespace pican {

// An Option either holds a value of type TP ("some") or holds nothing ("none"). It is the single-alternative sibling
// of pican::Result and deliberately mirrors its design and naming, see Result.hpp.
//
// Like Result, an Option is never default constructed or implicitly converted to, it is only ever created through
// its explicit factory functions (some_by_copy, some_by_move, some_emplace, some_default, none) so that every call
// site states exactly how the value gets into the Option: copied, moved, constructed in place, or not at all.
//
// Storage is a bool plus an anonymous union holding the value, the union lets us have correctly sized and aligned
// storage for a TP *without* constructing a TP, which is how "none" can exist for a TP that has no default
// constructor. This is the same trick std::optional uses internally. The cost is that the compiler no longer
// constructs, copies or destroys the value for us, so every one of the rule of 5 functions below is written by hand
// and must always check isSome_f before touching value_f.
template<typename TP>
class Option {
private:  // static assertions
    // The value lives inside a union, which cannot hold references, and void has no object to store. Arrays can be
    // stored but can't be copied or assigned with = so none of the copy-control below would work for them. Checking
    // here gives a readable error instead of one from deep inside the class
    static_assert(std::is_object_v<TP>, "Option value type must be an object type (not a reference, void or function)");
    static_assert(!std::is_array_v<TP>, "Option value type must not be an array");

private:  // types
    // Tag types, they carry no data and only exist to select a private constructor during overload resolution.
    //
    // SomeParameterTag is what stops the variadic emplace constructor below from hijacking copies: a constructor
    // template taking (Args_TP&&...) alone would, for a non-const Option lvalue, deduce Option& which is a better
    // match than the copy constructor's const Option&. Requiring the tag first means that can never happen.
    //
    // NoneParameterTag gives the none constructor a distinct signature so it can't be confused with any other
    struct SomeParameterTag {};

    struct NoneParameterTag {};

public:  // types
    using ValueType = TP;

private:  // member fields
    // Whether value_f is alive, the single source of truth for the state of the union. It is only ever changed
    // right after value_f's lifetime begins or ends so that it always describes what is actually alive
    bool hasValue_f;

    // Only alive when isSome_f is true, never read, assigned or destroyed otherwise
    union {
        ValueType value_f;
    };

private:  // constructors
    // All constructors are private, the public way to create an Option is through the factory functions, which exist
    // so that the creation method is spelled out at the call site

    // some_by_copy, one copy construction of ValueType
    explicit Option(const ValueType& v, SomeParameterTag tag) : hasValue_f(true), value_f(v) {
    }

    // some_by_move, one move construction of ValueType. ValueType&& here is NOT a forwarding reference, ValueType is
    // fixed by the class, not deduced by this constructor, so v is always an rvalue reference and std::move is correct
    explicit Option(ValueType&& v, SomeParameterTag tag) : hasValue_f(true), value_f(std::move(v)) {
    }

    // some_emplace and some_default, the value is constructed directly inside the union from args, no copies or moves
    // of ValueType at all. Args_TP&& IS a forwarding reference since Args_TP is deduced here, so std::forward keeps
    // each argument's value category. With an empty pack this is value_f() which value-initializes, so an int becomes
    // 0 rather than indeterminate
    template<typename... Args_TP>
    explicit Option(SomeParameterTag tag, Args_TP&&... args) :
        hasValue_f(true), value_f(std::forward<Args_TP>(args)...) {
    }

    // none, value_f is deliberately not in the initializer list, so no member of the union is alive and no ValueType
    // is ever constructed
    explicit Option(NoneParameterTag tag) : hasValue_f(false) {
    }

public:  // copy-control
    // Rule of 5, all written explicitly and none of them = default. When ValueType has a non-trivial constructor,
    // assignment or destructor, the defaulted versions are defined as deleted because the compiler cannot know whether
    // the union's member is alive, only isSome_f knows that. When ValueType is trivial they would compile, but they
    // would copy the storage without looking at isSome_f, correct only by accident.
    //
    // Each is constrained with requires on the matching capability of ValueType, so for example Option<MoveOnly> is
    // correctly reported as not copy constructible by std::is_copy_constructible_v, rather than claiming it is and then
    // failing to compile inside the body the moment someone tries.
    //
    // std::construct_at and std::destroy_at start and end the lifetime of value_f. They are exactly placement new and
    // an explicit destructor call, but they say what they do. std::addressof is used instead of & because ValueType
    // may overload operator&.

    // Copy constructor, if rhs holds a value we copy construct ours from it, otherwise we hold nothing too
    Option(const Option& rhs)
        requires(std::is_copy_constructible_v<ValueType>)
        : hasValue_f(rhs.hasValue_f) {
        if (rhs.hasValue_f) {
            std::construct_at(std::addressof(this->value_f), rhs.value_f);
        }
    }

    // Move constructor, same as the copy constructor but the value is moved from. rhs is left holding a moved-from
    // value, NOT turned into none, this is the same as std::optional and means moving never changes rhs's state, it
    // only changes what rhs's value contains. noexcept whenever ValueType's move is, so std::vector and friends will
    // move rather than copy an Option when they reallocate
    Option(Option&& rhs) noexcept(std::is_nothrow_move_constructible_v<ValueType>)
        requires(std::is_move_constructible_v<ValueType>)
        : hasValue_f(rhs.hasValue_f) {
        if (rhs.hasValue_f) {
            std::construct_at(std::addressof(this->value_f), std::move(rhs.value_f));
        }
    }

    // Copy assignment, there are 4 combinations of (this, rhs) states and each needs different handling because the
    // union's value is only sometimes alive:
    //
    //  some <- some: both alive, plain copy assignment of the value
    //  some <- none: ours must stop existing, destroy it
    //  none <- some: we have nothing to assign to (assigning to a dead object is undefined behaviour), so we copy
    //                construct a new value instead
    //  none <- none: nothing to do
    //
    // Requires both copy constructible (for none <- some) and copy assignable (for some <- some). Only callable on
    // lvalues (&), assigning to a temporary Option is almost certainly a bug
    Option&
    operator=(const Option& rhs) &
        requires(std::is_copy_constructible_v<ValueType> && std::is_copy_assignable_v<ValueType>)
    {
        contracts::postcondition([this, &rhs]() -> void {
            contracts::assertion(this->hasValue_f == rhs.hasValue_f);
        });
        // Self assignment, without this check some <- some would still be fine but it's cheaper to do nothing
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if (this->hasValue_f && rhs.hasValue_f) {
            this->value_f = rhs.value_f;
        } else if (this->hasValue_f && !rhs.hasValue_f) {
            std::destroy_at(std::addressof(this->value_f));
            this->hasValue_f = false;
        } else if (!this->hasValue_f && rhs.hasValue_f) {
            std::construct_at(std::addressof(this->value_f), rhs.value_f);
            this->hasValue_f = true;
        } else {
            // none <- none, nothing is alive on either side so there is nothing to do
        }
        return *this;
    }

    // Move assignment, the same 4 combinations as copy assignment but the value is moved from. As with the move
    // constructor, rhs keeps its state and only its value is moved from. noexcept when both of ValueType's moves are
    // since both can be called here
    Option&
    operator=(
        Option&& rhs
    ) & noexcept(std::is_nothrow_move_constructible_v<ValueType> && std::is_nothrow_move_assignable_v<ValueType>)
        requires(std::is_move_constructible_v<ValueType> && std::is_move_assignable_v<ValueType>)
    {
        contracts::postcondition([this, &rhs]() -> void {
            contracts::assertion(this->hasValue_f == rhs.hasValue_f);
        });
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if (this->hasValue_f && rhs.hasValue_f) {
            this->value_f = std::move(rhs.value_f);
        } else if (this->hasValue_f && !rhs.hasValue_f) {
            std::destroy_at(std::addressof(this->value_f));
            this->hasValue_f = false;
        } else if (!this->hasValue_f && rhs.hasValue_f) {
            std::construct_at(std::addressof(this->value_f), std::move(rhs.value_f));
            this->hasValue_f = true;
        } else {
            // none <- none, nothing is alive on either side so there is nothing to do
        }
        return *this;
    }

    // Destructor, the union will not destroy value_f for us so we must, but only if it's alive, destroying a value
    // that was never constructed is undefined behaviour
    ~Option() {
        if (this->hasValue_f) {
            std::destroy_at(std::addressof(this->value_f));
        }
    }

public:  // factory functions
    // All [[nodiscard]], creating an Option and throwing it away is always a mistake

    // The value is copy constructed from v
    [[nodiscard]]
    static Option
    some_by_copy(const ValueType& v) {
        return Option{v, SomeParameterTag{}};
    }

    // The value is move constructed from v
    [[nodiscard]]
    static Option
    some_by_move(ValueType&& v) {
        return Option{std::move(v), SomeParameterTag{}};
    }

    // The value is constructed in place from args, no copies or moves of ValueType
    template<typename... Args_TP>
    [[nodiscard]]
    static Option
    some_emplace(Args_TP&&... args) {
        return Option{SomeParameterTag{}, std::forward<Args_TP>(args)...};
    }

    // The value is value-initialized in place, ValueType{} for class types, 0 for arithmetic types
    [[nodiscard]]
    static Option
    some_default() {
        return Option{SomeParameterTag{}};
    }

    // No value, no ValueType is constructed
    [[nodiscard]]
    static Option
    none() {
        return Option{NoneParameterTag{}};
    }

public:  // member functions
    [[nodiscard]]
    bool
    has_value() const {
        return this->hasValue_f;
    }

    [[nodiscard]]
    bool
    is_empty() const {
        return !this->hasValue_f;
    }

    // Returns the value if some, otherwise defaultValue, either way a reference is returned so nothing is copied.
    // Because the return is a reference, it can point at either our value or at defaultValue, which means BOTH must
    // outlive the returned reference, the deleted overloads below enforce that at compile time

    [[nodiscard]]
    const ValueType&
    value_or_else(const ValueType& defaultValue) const& {
        if (this->hasValue_f) {
            return this->value_f;
        } else {
            return defaultValue;
        }
    }

    [[nodiscard]]
    ValueType&
    value_or_else(ValueType& defaultValue) & {
        if (this->hasValue_f) {
            return this->value_f;
        } else {
            return defaultValue;
        }
    }

    // A temporary defaultValue dies at the end of the full expression, so on none we'd return a dangling reference.
    // Deleting the rvalue overloads makes the compiler choose these for temporaries (rvalues prefer rvalue references
    // in overload resolution) and then refuse to compile
    const ValueType&
    value_or_else(const ValueType&& defaultValue) const& = delete;

    ValueType&
    value_or_else(ValueType&& defaultValue) & = delete;

    // A temporary Option dies at the end of the full expression, so on some we'd return a dangling reference to our
    // value. const&& catches both const and non-const temporaries, because a non-const rvalue also prefers const&&
    // over const&
    const ValueType&
    value_or_else(const ValueType& defaultValue) const&& = delete;

    // Returns the value if some, otherwise panics, use when none means a bug rather than an expected outcome

    [[nodiscard]]
    const ValueType&
    value_or_panic() const& {
        if (this->hasValue_f) {
            return this->value_f;
        }
        pican::panic("Option was none, expected some");
    }

    [[nodiscard]]
    ValueType&
    value_or_panic() & {
        if (this->hasValue_f) {
            return this->value_f;
        }
        pican::panic("Option was none, expected some");
    }

    // Deleted for temporaries for the same dangling reason as value_or_else, use value_extract_or_panic instead
    [[nodiscard]]
    const ValueType&
    value_or_panic() const&& = delete;

    // Only callable on rvalues, e.g. std::move(option).value_extract_or_panic() or directly on a returned Option, and
    // gives back an rvalue reference to our value so the caller can move construct from it. The Option stays some,
    // holding a moved-from value, exactly as with the move constructor. Bind the result to a value, not a reference:
    // `ValueType v = f().value_extract_or_panic();` is fine, `auto&& v = f().value_extract_or_panic();` dangles
    [[nodiscard]]
    ValueType&&
    value_extract_or_panic() && {
        if (this->hasValue_f) {
            return std::move(this->value_f);
        }
        pican::panic("Option was none, expected some");
    }
};

}  // namespace pican
