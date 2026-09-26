#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "pican/core/functions.hpp"
#include "pican/core/types.hpp"

namespace pican {

// A heapless replacement for std::function: holds any callable (lambda with or without captures, function pointer,
// member function pointer, functor object) with a given call signature, inside a fixed size buffer that lives
// directly inside the Function object itself. It never allocates.
//
// Usage:
//
//     pican::Function<int(int, int), 32> add = [offset](int a, int b) { return a + b + offset; };
//     int three = add(1, 2);
//
// The first template parameter is the call signature, written like a function type, exactly like std::function. The
// second is the capacity in bytes of the internal buffer. A callable larger than the capacity is a compile error, not
// a heap allocation, so you always know the exact size of a Function from its type.
//
// ---------------------------------------------------------------------------------------------------------------------
// How it works: type erasure
// ---------------------------------------------------------------------------------------------------------------------
//
// Every lambda has its own unique, unnameable type, so to store "any lambda" in one type we must forget ("erase") the
// lambda's type while still being able to call, copy, move and destroy it later. The trick is:
//
//  1. When a callable of type C is given to the constructor, C IS still known, so we placement-construct the C inside
//     our byte buffer, and we instantiate a few small static functions that know C, for example:
//
//         static Return invoke_stored<C>(void* storage, Args&&... args) {
//             C& callable = *static_cast<C*>(storage);
//             return callable(args...);
//         }
//
//  2. We keep pointers to those functions as members. Their types don't mention C at all, they only take a void*,
//     so every Function<Sig, N> has the same layout no matter what callable it holds. This is where C gets erased.
//
//  3. Later, calling the Function calls through the stored function pointer, which casts the buffer back to C (it's
//     the only code that knows what C is) and calls it.
//
// ---------------------------------------------------------------------------------------------------------------------
// Performance choices
// ---------------------------------------------------------------------------------------------------------------------
//
//  - Exactly one indirect call per invocation. The invoke function pointer is stored directly in the Function object,
//    not behind a vtable. A virtual-function-based design would first load the vtable pointer and then load the
//    function pointer from the vtable, 2 dependent loads before the call. Here it's one load then the call. And
//    because the callable lives inside the Function, reaching it costs no pointer chase either.
//
//  - No "is empty" branch when calling. An empty Function's invoke pointer points at invoke_empty which panics, so
//    operator() never has to check for emptiness, it always just calls the pointer.
//
//  - The rare operations (copy, move, destroy) share a single second function pointer, manage_f, which takes an
//    operation enum. This costs one pointer in the object rather than three. These operations are much rarer than
//    calls, so the switch inside is a fine trade.
//
//  - Trivially copyable callables pay nothing for copy, move or destroy. Function pointers, captureless lambdas and
//    lambdas capturing only trivially copyable things (ints, pointers, references) are the common case. For them
//    manage_f is nullptr, so copy and move are a fixed size memcpy of the buffer (which the compiler inlines) and
//    destroying does nothing. No indirect call at all.
//
// ---------------------------------------------------------------------------------------------------------------------
// Semantics worth knowing
// ---------------------------------------------------------------------------------------------------------------------
//
//  - operator() is const but calls the stored callable as NON-const, the same as std::function. That is what lets a
//    mutable lambda (one that changes its own captures) be called through a const Function. It's a known const
//    correctness hole in std::function's design, kept here for the same convenience.
//
//  - A moved-from Function is always empty. std::function leaves this unspecified, here it's guaranteed.
//
//  - Callables must be copy constructible (because Function itself is copyable) and nothrow move constructible
//    (so that Function's own move can be noexcept). Lambdas capturing a move-only type like std::unique_ptr aren't
//    accepted, that would need a separate move-only Function type, like C++23's std::move_only_function.
//
//  - Calling an empty Function panics.
//
// A Function<Sig, N> can store a Function<Sig, M> of a smaller capacity, because a Function is itself a callable, but
// it then stores the whole inner Function and a call goes through 2 indirect calls instead of 1.

// Declared but never defined, only the specialization below for function types exists. This is the standard trick for
// taking a signature like int(int, int) as one template parameter and then pulling it apart into its return type and
// argument types. Function<int, 32> (a non function type) matches only this primary template and so is an error
template<typename Signature_TP, pican::SizeBytes Capacity_V>
class Function;

// The partial specialization for function types, Return_TP(Args_TP...) is pattern matched against the signature, so
// for Function<int(float, char), 32>, Return_TP = int and Args_TP = {float, char}. The order of this template's own
// parameters doesn't have to match the primary template's, they're only names for the pattern's pieces, Capacity_V is
// first only so that the pack Args_TP comes last, which keeps it readable
template<pican::SizeBytes Capacity_V, typename Return_TP, typename... Args_TP>
class Function<Return_TP(Args_TP...), Capacity_V> {
private:  // static assertions
    // A zero sized array isn't valid C++, and nothing useful fits in zero bytes anyway
    static_assert(Capacity_V > 0, "Function capacity must be at least 1 byte");

public:  // types
    using ReturnType = Return_TP;

public:  // constants
    // Size of the internal buffer in bytes, the largest callable that can be stored
    static constexpr pican::SizeBytes CAPACITY = Capacity_V;

    // Alignment of the internal buffer, the largest alignment that a callable can require. alignof(std::max_align_t)
    // is the alignment of the most aligned fundamental type (16 on x86-64 and aarch64), which is what malloc
    // guarantees too, so any ordinary type fits. Only types explicitly declared with a larger alignas are rejected
    static constexpr pican::Alignment ALIGNMENT = alignof(std::max_align_t);

private:  // types
    // Type of the function that calls the stored callable. The storage is passed as void* because this type must not
    // mention the callable's type, that's the whole point of type erasure.
    //
    // The arguments are taken as Args_TP&& (references) rather than by value so that passing them through here
    // costs nothing: operator() takes its arguments by value (as dictated by the signature), and then only
    // references to them are passed along, until the callable itself receives them. Note that since Args_TP is fixed
    // by the class and not deduced by this function, Args_TP&& is NOT a forwarding reference: for Args_TP = int it's
    // int&&, for Args_TP = int& it collapses to int&
    using InvokeFunction = Return_TP (*)(void* storage, Args_TP&&... args);

    // The rare operations on a stored callable, all handled by the one manage function
    enum class ManageOperation : std::uint8_t {
        // Copy construct a callable in destination from the one in source, source is left untouched
        COPY,
        // Move construct a callable in destination from the one in source, then destroy the one in source
        MOVE,
        // Destroy the callable in destination, source is unused
        DESTROY,
    };

    // Type of the function that copies, moves or destroys the stored callable, see ManageOperation. noexcept is part
    // of a function pointer's type since C++17, so only noexcept functions can be stored in one of these
    using ManageFunction = void (*)(ManageOperation operation, void* destination, void* source) noexcept;

private:  // member fields
    // The buffer that the callable lives in. alignas makes the buffer's address suitably aligned for any callable
    // that passes the ALIGNMENT check. It's placed first so that its alignment doesn't cause padding between members.
    //
    // std::byte is used because it is one of the types (with char and unsigned char) that the standard allows to
    // provide storage for other objects. Placement-constructing a callable here begins that object's lifetime inside
    // the buffer.
    //
    // mutable because operator() is const (see the semantics note above) but must pass a non-const pointer to the
    // callable. mutable says that honestly in the declaration, instead of a const_cast at each use
    alignas(ALIGNMENT) mutable std::byte storage_f[CAPACITY];

    // Calls the stored callable. Never nullptr: when empty it points at invoke_empty
    InvokeFunction invoke_f;

    // Copies, moves or destroys the stored callable. nullptr when the callable is trivially copyable and trivially
    // destructible (or when empty), which means copy and move are memcpy and destroy is nothing
    ManageFunction manage_f;

private:  // static functions
    // The functions below are static member function templates. Each instantiation, such as invoke_stored<SomeLambda>,
    // is a separate ordinary function with its own address, generated only if it's used. They are the only code that
    // knows the stored callable's real type.

    // Recovers a typed pointer to the callable stored in storage.
    //
    // std::launder is needed because storage points at the std::byte buffer, not at the Callable_TP object that was
    // placement-constructed inside it. The two share an address, but the standard says a pointer obtained by casting
    // the buffer's address still points at the buffer. std::launder tells the compiler "there is a Callable_TP object
    // alive at this address, give me a pointer to it". In practice it compiles to nothing, it only stops the optimizer
    // from making assumptions that would be wrong here
    template<typename Callable_TP>
    static Callable_TP*
    stored_callable(void* storage) {
        return std::launder(static_cast<Callable_TP*>(storage));
    }

    // Calls the Callable_TP stored in storage with args.
    //
    // std::invoke_r (C++23) handles every kind of callable uniformly: lambdas and functors via operator(), function
    // pointers via a plain call, and member function pointers via (object.*pointer)(rest...) where the first
    // argument is the object. It also converts the result to Return_TP, or discards it when Return_TP is void, so a
    // callable returning int can be stored in a Function<void()>
    template<typename Callable_TP>
    static Return_TP
    invoke_stored(void* storage, Args_TP&&... args) {
        Callable_TP& callable = *stored_callable<Callable_TP>(storage);
        return std::invoke_r<Return_TP>(callable, std::forward<Args_TP>(args)...);
    }

    // What an empty Function's invoke_f points at, so operator() never needs to branch on emptiness. [[noreturn]]
    // because panic never returns, which is also why there's no return statement despite Return_TP maybe being
    // non-void
    [[noreturn]]
    static Return_TP
    invoke_empty(void* /* storage */, Args_TP&&... /* args */) {
        pican::panic("Called an empty pican::Function");
    }

    // Copies, moves or destroys the Callable_TP stored in source/destination, see ManageOperation. Only used for
    // callables that are not trivially copyable or not trivially destructible, the others need none of this.
    //
    // noexcept because ManageFunction requires it. The copy constructor of a callable is allowed to throw, and if one
    // did here, std::terminate would be called. This codebase is built with -fno-exceptions, so that cannot happen
    template<typename Callable_TP>
    static void
    manage_stored(ManageOperation operation, void* destination, void* source) noexcept {
        switch (operation) {
            case ManageOperation::COPY: {
                // destination holds no object yet, so it's cast directly rather than laundered, construct_at is what
                // begins the new object's lifetime there
                // std::as_const so that the copy constructor is chosen even if Callable_TP has a greedy template
                // constructor that would be a better match for a non-const lvalue
                std::construct_at(static_cast<Callable_TP*>(destination),
                                  std::as_const(*stored_callable<Callable_TP>(source)));
                return;
            }
            case ManageOperation::MOVE: {
                Callable_TP* const sourceCallable = stored_callable<Callable_TP>(source);
                std::construct_at(static_cast<Callable_TP*>(destination), std::move(*sourceCallable));
                // A moved-from object is still alive and must still be destroyed, the move doesn't end its lifetime
                std::destroy_at(sourceCallable);
                return;
            }
            case ManageOperation::DESTROY: {
                std::destroy_at(stored_callable<Callable_TP>(destination));
                return;
            }
        }
    }

    // Whether a callable can skip manage_stored: copying a trivially copyable object is by definition the same as
    // copying its bytes, and destroying a trivially destructible object is by definition doing nothing
    template<typename Callable_TP>
    static constexpr bool IS_TRIVIAL =
        std::is_trivially_copyable_v<Callable_TP> && std::is_trivially_destructible_v<Callable_TP>;

public:  // constructors
    // An empty Function, calling it panics. Default constructible, unlike Result and Option, because an empty Function
    // is a normal state for, say, a callback member that hasn't been set yet
    Function() : invoke_f(&invoke_empty), manage_f(nullptr) {
    }

    // Stores any callable that can be called with Args_TP... and gives something convertible to Return_TP.
    //
    // Callable_TP&& IS a forwarding reference (Callable_TP is deduced by this constructor): an lvalue lambda is
    // copied into the buffer and an rvalue (temporary) lambda is moved into it.
    //
    // The requires clause is what makes this template safe to have:
    //  - The first check excludes Function itself. Without it, copying a non-const Function lvalue would choose this
    //    template (Callable_TP = Function&, an exact match) over the copy constructor (const Function&, needs a const
    //    conversion), and the Function would be wrapped inside itself instead of being copied.
    //  - The second check rejects callables with the wrong signature, so they fail overload resolution with a clear
    //    "constraints not satisfied" error, and so that std::is_constructible_v and friends report the truth.
    // Conditions in a requires clause joined by && are checked left to right and stop at the first false one, so the
    // invocability check is never even looked at for Function itself.
    //
    // std::decay_t turns what was passed into the type to store: it strips references and const, and turns a
    // function (passing a function's name, like `my_function`) into a function pointer.
    //
    // Deliberately NOT explicit, same as std::function, so a lambda can be passed straight to a parameter of type
    // Function without being wrapped by hand. This is safe because a callable that doesn't fit is a compile error.
    template<typename Callable_TP>
        requires(!std::is_same_v<std::remove_cvref_t<Callable_TP>, Function>) &&
                (std::is_invocable_r_v<Return_TP, std::decay_t<Callable_TP>&, Args_TP...>)
    Function(Callable_TP&& callable) : invoke_f(&invoke_empty), manage_f(nullptr) {
        this->store(std::forward<Callable_TP>(callable));
    }

public:  // copy-control
    // Rule of 5, all written explicitly. The buffer is just bytes, the compiler doesn't know what's in it, so every
    // one of these has to ask the stored callable (through manage_f) to copy, move or destroy itself, or, for trivial
    // callables, copy the bytes

    // Copy constructor, the new Function holds a copy of rhs's callable
    Function(const Function& rhs) : invoke_f(rhs.invoke_f), manage_f(rhs.manage_f) {
        this->copy_storage_from(rhs);
    }

    // Move constructor, the callable moves into the new Function and rhs becomes empty. noexcept is guaranteed because
    // only nothrow move constructible callables are accepted
    Function(Function&& rhs) noexcept : invoke_f(rhs.invoke_f), manage_f(rhs.manage_f) {
        this->move_storage_from(rhs);
    }

    // Copy assignment, whatever we held is destroyed first, then we copy rhs's callable
    Function&
    operator=(const Function& rhs) & {
        // Self assignment check is required here, not just an optimization: without it reset() would destroy the very
        // callable we're about to copy from
        if (this == std::addressof(rhs)) {
            return *this;
        }
        this->reset();
        this->invoke_f = rhs.invoke_f;
        this->manage_f = rhs.manage_f;
        this->copy_storage_from(rhs);
        return *this;
    }

    // Move assignment, whatever we held is destroyed first, then rhs's callable moves into us and rhs becomes empty
    Function&
    operator=(Function&& rhs) & noexcept {
        if (this == std::addressof(rhs)) {
            return *this;
        }
        this->reset();
        this->invoke_f = rhs.invoke_f;
        this->manage_f = rhs.manage_f;
        this->move_storage_from(rhs);
        return *this;
    }

    // Destructor, only non-trivial callables need anything done, for trivial ones and empty Functions manage_f is
    // nullptr and this is a single comparison
    ~Function() {
        if (this->manage_f != nullptr) {
            this->manage_f(ManageOperation::DESTROY, this->storage_f, nullptr);
        }
    }

    // Replaces the stored callable with a new one, the same constraints as the converting constructor and for the same
    // reasons, the first check stops this from hijacking copy and move assignment
    template<typename Callable_TP>
        requires(!std::is_same_v<std::remove_cvref_t<Callable_TP>, Function>) &&
                (std::is_invocable_r_v<Return_TP, std::decay_t<Callable_TP>&, Args_TP...>)
    Function&
    operator=(Callable_TP&& callable) & {
        this->reset();
        this->store(std::forward<Callable_TP>(callable));
        return *this;
    }

private:  // member functions
    // Puts callable into the buffer and points invoke_f and manage_f at the functions for its type. Must only be called
    // when empty, it doesn't destroy what was there. This is the one place the callable's real type is known and all
    // the checks on it are made
    template<typename Callable_TP>
    void
    store(Callable_TP&& callable) {
        using Stored_TP = std::decay_t<Callable_TP>;

        // These are static_asserts rather than part of the requires clause on purpose: the requires clause answers
        // "is this a callable of the right signature?", which should just remove this overload. "It's the right kind
        // of callable but too big" is a mistake the programmer needs to hear about with a clear message. Clang and
        // recent GCC also print the evaluated sizes when one of these fails
        static_assert(sizeof(Stored_TP) <= CAPACITY,
                      "Callable is too large for this Function's capacity, increase the capacity or capture less");
        static_assert(alignof(Stored_TP) <= ALIGNMENT, "Callable is over-aligned for this Function's buffer");
        static_assert(std::is_copy_constructible_v<Stored_TP>,
                      "Callable must be copy constructible because Function is copyable");
        static_assert(std::is_nothrow_move_constructible_v<Stored_TP>,
                      "Callable must be nothrow move constructible so that Function's move can be noexcept");

        // A null function pointer or null member function pointer is stored as an empty Function, the same as
        // std::function does, so calling it panics clearly instead of jumping to address 0. The check is on the type
        // as passed (remove_cvref_t) and not the decayed type: passing a function by name (a function reference) can
        // never be null, and comparing it with nullptr would get a -Waddress warning
        if constexpr (std::is_pointer_v<std::remove_cvref_t<Callable_TP>> ||
                      std::is_member_pointer_v<std::remove_cvref_t<Callable_TP>>) {
            if (callable == nullptr) {
                return;
            }
        }

        std::construct_at(reinterpret_cast<Stored_TP*>(this->storage_f), std::forward<Callable_TP>(callable));
        this->invoke_f = &invoke_stored<Stored_TP>;
        if constexpr (IS_TRIVIAL<Stored_TP>) {
            this->manage_f = nullptr;
        } else {
            this->manage_f = &manage_stored<Stored_TP>;
        }
    }

    // Copies the callable in rhs's buffer into ours, invoke_f and manage_f must already be copied from rhs, and we must
    // hold nothing
    void
    copy_storage_from(const Function& rhs) {
        if (rhs.manage_f != nullptr) {
            rhs.manage_f(ManageOperation::COPY, this->storage_f, rhs.storage_f);
        } else if (!rhs.is_empty()) {
            // A trivial callable, copying its bytes is copying it. The whole buffer is copied, not just the callable's
            // size (which we no longer know), and since CAPACITY is a compile time constant the compiler turns this
            // into a few plain moves. Bytes past the end of the callable are indeterminate, copying indeterminate bytes
            // is allowed for std::byte. Copying the bytes of a trivially copyable type implicitly creates the object in
            // the destination (C++20 implicit object creation), so the callable is properly alive in our buffer after
            // this. Skipped when empty: there's nothing to copy, and the whole buffer would be indeterminate
            std::memcpy(this->storage_f, rhs.storage_f, CAPACITY);
        }
    }

    // Moves the callable in rhs's buffer into ours and makes rhs empty, invoke_f and manage_f must already be copied
    // from rhs, and we must hold nothing
    void
    move_storage_from(Function& rhs) noexcept {
        if (rhs.manage_f != nullptr) {
            // Moves into us and also destroys rhs's moved-from callable
            rhs.manage_f(ManageOperation::MOVE, this->storage_f, rhs.storage_f);
        } else if (!rhs.is_empty()) {
            // A trivial callable, see copy_storage_from, and rhs's copy needs no destruction since it's trivial
            std::memcpy(this->storage_f, rhs.storage_f, CAPACITY);
        }
        // rhs's callable is gone (moved and destroyed, or trivial), so mark it empty
        rhs.invoke_f = &invoke_empty;
        rhs.manage_f = nullptr;
    }

public:  // member functions
    // Calls the stored callable, panics if empty. One indirect call, no branches, see the performance notes at the top.
    //
    // Arguments are taken as the signature says (by value for Function<void(std::string)>), then forwarded as
    // references through invoke_f to the callable, so a by value argument is moved exactly once more at most, into the
    // callable's own parameter. std::forward<Args_TP> here turns by value parameters into rvalues (they are ours and
    // about to die) and leaves reference parameters as they were
    Return_TP
    operator()(Args_TP... args) const {
        return this->invoke_f(this->storage_f, std::forward<Args_TP>(args)...);
    }

    // Whether no callable is stored, true for a default constructed Function, a moved-from one, a reset one, or one
    // constructed from a null function pointer
    [[nodiscard]]
    bool
    is_empty() const {
        return this->invoke_f == &invoke_empty;
    }

    // Destroys the stored callable, if any, and makes this empty
    void
    reset() {
        if (this->manage_f != nullptr) {
            this->manage_f(ManageOperation::DESTROY, this->storage_f, nullptr);
        }
        this->invoke_f = &invoke_empty;
        this->manage_f = nullptr;
    }
};

}  // namespace pican
