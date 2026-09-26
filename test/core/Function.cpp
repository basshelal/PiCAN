#include "pican/core/Function.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include <catch2/catch_all.hpp>

#include "heap/Heap.hpp"
#include "test/utils/test_utils.hpp"

using pican::test_utils::LifetimeOperation;
using pican::test_utils::Tracked;

using ValueType = Tracked<std::string>;

// Enough for any function pointer (1 pointer), member function pointer (2 pointers on the Itanium ABI used by GCC and
// Clang on Linux) and a lambda capturing a few ints, pointers or references
constexpr pican::SizeBytes SMALL_CAPACITY = 4 * sizeof(void*);

using BinaryFunction = pican::Function<int(int, int), SMALL_CAPACITY>;

namespace {

int
add(int a, int b) {
    return a + b;
}

int
multiply(int a, int b) {
    return a * b;
}

// For member function pointers
struct Counter {
    int value;

    int
    get() const {
        return this->value;
    }

    void
    increase(int amount) {
        this->value += amount;
    }
};

// Takes a Function by const reference, to show a lambda or function converting to one implicitly at a call site
int
call_with_one_and_two(const BinaryFunction& function) {
    return function(1, 2);
}

// A callable of the wrong argument types for BinaryFunction, only used in type traits
struct TakesString {
    int
    operator()(const std::string& s) const {
        return static_cast<int>(s.size());
    }
};

// A callable of the right argument types but a return type that doesn't convert to int, only used in type traits
struct ReturnsString {
    std::string
    operator()(int a, int b) const {
        return std::to_string(a + b);
    }
};

// Doesn't allocate but isn't trivially copyable or trivially destructible, because its copy constructor and destructor
// are user-provided (written by hand, even if they do what the defaults would). A lambda capturing one is therefore
// also not trivially copyable, and a Function holding that lambda has to go through manage_f to copy, move and destroy
// it, rather than memcpy. Used to test that path without the heap
struct NotTriviallyCopyable {
    int value;

    explicit NotTriviallyCopyable(int v) : value(v) {
    }

    NotTriviallyCopyable(const NotTriviallyCopyable& rhs) : value(rhs.value) {
    }

    NotTriviallyCopyable(NotTriviallyCopyable&& rhs) noexcept : value(rhs.value) {
    }

    NotTriviallyCopyable&
    operator=(const NotTriviallyCopyable& rhs) = default;

    NotTriviallyCopyable&
    operator=(NotTriviallyCopyable&& rhs) noexcept = default;

    ~NotTriviallyCopyable() {
    }
};

static_assert(!std::is_trivially_copyable_v<NotTriviallyCopyable>);
static_assert(!std::is_trivially_destructible_v<NotTriviallyCopyable>);

}  // namespace

// Compile time checks. These are static_asserts, so if one fails the test binary doesn't build. They check the parts
// of Function's contract that are about what code compiles rather than what it does at runtime
namespace {

// Copy-control, Function is fully copyable and its moves are noexcept regardless of what it holds
static_assert(std::is_default_constructible_v<BinaryFunction>);
static_assert(std::is_copy_constructible_v<BinaryFunction>);
static_assert(std::is_copy_assignable_v<BinaryFunction>);
static_assert(std::is_nothrow_move_constructible_v<BinaryFunction>);
static_assert(std::is_nothrow_move_assignable_v<BinaryFunction>);

// Accepts function pointers, functions by name (a function reference, which decays to a pointer), and any callable
// with a matching signature. The converting constructor isn't explicit, so conversion is implicit
static_assert(std::is_constructible_v<BinaryFunction, decltype(&add)>);
static_assert(std::is_constructible_v<BinaryFunction, decltype(add)&>);
static_assert(std::is_convertible_v<decltype(&add), BinaryFunction>);

// Rejects callables that can't be called with (int, int), or whose result doesn't convert to int, and things that
// aren't callable at all. These are rejected by the requires clause, which is why the traits can see it
static_assert(!std::is_constructible_v<BinaryFunction, TakesString>);
static_assert(!std::is_constructible_v<BinaryFunction, ReturnsString>);
static_assert(!std::is_constructible_v<BinaryFunction, int>);

// A void returning Function accepts any return type, the result is discarded, the same as std::function
static_assert(std::is_constructible_v<pican::Function<void(int, int), SMALL_CAPACITY>, ReturnsString>);

// Note that "too big for the capacity" can NOT be checked with a trait like the above. That's a static_assert inside
// the constructor's body, which the traits never instantiate, they only look at the declaration (and its requires
// clause). So std::is_constructible_v says true, and actually constructing is a compile error with a clear message

// The Function is its buffer plus 2 pointers (invoke_f and manage_f), rounded up to the buffer's alignment and nothing
// else, in particular there is no heap pointer and no hidden allocation
static_assert(alignof(BinaryFunction) == alignof(std::max_align_t));
static_assert(sizeof(BinaryFunction) >= SMALL_CAPACITY + 2 * sizeof(void*));
static_assert(sizeof(BinaryFunction) < SMALL_CAPACITY + 2 * sizeof(void*) + alignof(std::max_align_t));

}  // namespace

TEST_CASE("Function") {
    SECTION("Default constructed is empty") {
        const BinaryFunction function;
        CHECK(function.is_empty());

        const BinaryFunction copy{function};
        CHECK(copy.is_empty());
        // Calling an empty Function panics, which aborts the whole test process, so that isn't tested here
    }

    SECTION("Function pointer") {
        // By name, add is a function reference that decays to a pointer
        const BinaryFunction byName{add};
        CHECK_FALSE(byName.is_empty());
        CHECK(byName(2, 3) == 5);

        // By explicitly taking its address, the same thing
        const BinaryFunction byPointer{&add};
        CHECK_FALSE(byPointer.is_empty());
        CHECK(byPointer(2, 3) == 5);
    }

    SECTION("Null function pointer is empty") {
        // Stored as empty rather than as a pointer to address 0, so calling it panics clearly rather than crashing
        int (*nullPointer)(int, int) = nullptr;
        const BinaryFunction function{nullPointer};
        CHECK(function.is_empty());
    }

    SECTION("Member function pointer") {
        // The object is the first argument, std::invoke_r turns getter(counter) into (counter.*pointer)()
        const pican::Function<int(const Counter&), SMALL_CAPACITY> getter{&Counter::get};
        const Counter counter{.value = 7};
        CHECK(getter(counter) == 7);

        const pican::Function<void(Counter&, int), SMALL_CAPACITY> increaser{&Counter::increase};
        Counter mutableCounter{.value = 1};
        increaser(mutableCounter, 2);
        CHECK(mutableCounter.value == 3);
    }

    SECTION("Captureless lambda") {
        const BinaryFunction function = [](int a, int b) {
            return a - b;
        };
        CHECK(function(5, 3) == 2);
    }

    SECTION("Lambda capturing by value") {
        // Not const: capturing a const int initialized with a constant makes the capture unnecessary, and Clang warns
        // about unnecessary captures
        int offset = 10;
        const BinaryFunction function = [offset](int a, int b) {
            return a + b + offset;
        };
        // The capture is a copy, changing the original afterwards doesn't affect it
        offset = 1000;
        CHECK(function(1, 2) == 13);
    }

    SECTION("Lambda capturing by reference") {
        int total = 0;
        const pican::Function<void(int), SMALL_CAPACITY> accumulate = [&total](int amount) {
            total += amount;
        };
        accumulate(2);
        accumulate(3);
        CHECK(total == 5);
    }

    SECTION("Mutable lambda keeps its state and copies are independent") {
        pican::Function<int(), SMALL_CAPACITY> counter = [count = 0]() mutable {
            return ++count;
        };
        CHECK(counter() == 1);
        CHECK(counter() == 2);

        // The copy has its own copy of count, starting from where the original was when it was copied
        const pican::Function<int(), SMALL_CAPACITY> copy{counter};
        // Calling a mutable lambda through a const Function works, the same as std::function, see Function.hpp
        CHECK(copy() == 3);
        CHECK(copy() == 4);
        // The original wasn't affected by the copy's calls
        CHECK(counter() == 3);
    }

    SECTION("Implicit conversion at a call site") {
        CHECK(call_with_one_and_two([](int a, int b) { return a * 10 + b; }) == 12);
        CHECK(call_with_one_and_two(multiply) == 2);
    }

    SECTION("Return type conversion") {
        // The callable returns short, the Function returns long, std::invoke_r converts
        const pican::Function<long(int), SMALL_CAPACITY> widen = [](int a) -> short {
            return static_cast<short>(a);
        };
        CHECK(widen(42) == 42L);

        // The callable returns int, the Function returns void, the result is discarded
        int calls = 0;
        const pican::Function<void(), SMALL_CAPACITY> discard = [&calls]() {
            calls++;
            return 42;
        };
        discard();
        CHECK(calls == 1);
    }

    SECTION("Reference arguments") {
        // Args_TP = int&, so the reference is passed all the way through to the lambda and it modifies our variable
        const pican::Function<void(int&), SMALL_CAPACITY> doubler = [](int& value) {
            value *= 2;
        };
        int value = 4;
        doubler(value);
        CHECK(value == 8);
    }

    SECTION("By value arguments are never copied more than the caller asks for") {
        int copies = 0;
        const ValueType::LifetimeCallbacks callbacks{.onCopyConstructed = [&copies](const ValueType&) {
            copies++;
        }};
        const pican::Function<LifetimeOperation(ValueType), SMALL_CAPACITY> lastOperationOf = [](ValueType v) {
            return v.lastOperation;
        };

        // Passing an rvalue: moved into operator()'s parameter, passed by reference through invoke_f, then moved into
        // the lambda's parameter. No copies at all
        ValueType moveArgument{std::string{"data"}, callbacks};
        CHECK(lastOperationOf(std::move(moveArgument)) == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(copies == 0);

        // Passing an lvalue: copied into operator()'s parameter (which the caller asked for by passing an lvalue to a
        // by value parameter), then moved the rest of the way. Exactly one copy
        const ValueType copyArgument{std::string{"data"}, callbacks};
        CHECK(lastOperationOf(copyArgument) == LifetimeOperation::MOVE_CONSTRUCTOR);
        CHECK(copies == 1);
    }

    SECTION("Copy, move and assignment") {
        int offset = 100;
        BinaryFunction source = [offset](int a, int b) {
            return a + b + offset;
        };

        SECTION("Move constructor leaves the source empty") {
            const BinaryFunction moved{std::move(source)};
            CHECK(moved(1, 2) == 103);
            // Guaranteed by Function (std::function leaves it unspecified), reading the moved-from source is deliberate
            CHECK(source.is_empty());
        }

        SECTION("Copy assignment") {
            BinaryFunction target{multiply};
            target = source;
            CHECK(target(1, 2) == 103);
            // A copy must leave its source untouched
            CHECK(source(1, 2) == 103);
        }

        SECTION("Move assignment leaves the source empty") {
            BinaryFunction target{multiply};
            target = std::move(source);
            CHECK(target(1, 2) == 103);
            CHECK(source.is_empty());
        }

        SECTION("Assigning a callable") {
            BinaryFunction target{multiply};
            target = add;
            CHECK(target(2, 3) == 5);
            target = [](int a, int b) {
                return a - b;
            };
            CHECK(target(2, 3) == -1);
        }

        SECTION("Reset") {
            source.reset();
            CHECK(source.is_empty());
            // Resetting an empty Function is fine and does nothing
            source.reset();
            CHECK(source.is_empty());
        }

        SECTION("Self assignment") {
            // Through references so the compiler doesn't warn about the obvious self assignment. Without the this ==
            // &rhs check, reset() would destroy the callable before it was copied from
            const BinaryFunction& selfConst = source;
            source = selfConst;
            CHECK(source(1, 2) == 103);

            BinaryFunction& selfMut = source;
            source = std::move(selfMut);
            CHECK(source(1, 2) == 103);
        }
    }
}

// Checks that Function starts and ends the lifetime of a non-trivial callable exactly once each, through manage_f.
// The callable is a lambda capturing a Tracked, so the lambda's copy, move and destruction are the Tracked's.
// aliveCount goes up whenever a Tracked is constructed (in any way) and down whenever one is destroyed, so if Function
// ever forgets to destroy its callable, destroys it twice, or destroys one it never constructed, the count ends up wrong
TEST_CASE("Function lifetimes are balanced") {
    int aliveCount = 0;
    const ValueType::LifetimeCallback onStarted = [&aliveCount](const ValueType&) {
        aliveCount++;
    };
    const ValueType::LifetimeCallback onEnded = [&aliveCount](const ValueType&) {
        aliveCount--;
    };
    const ValueType::LifetimeCallbacks callbacks{
        .onConstructed = onStarted,
        .onCopyConstructed = onStarted,
        .onMoveConstructed = onStarted,
        .onDestructed = onEnded,
    };

    // The lambda is exactly as big as its one capture on every mainstream compiler, the extra is just headroom
    using TrackedFunction = pican::Function<std::size_t(), sizeof(ValueType) + SMALL_CAPACITY>;

    // Creates a TrackedFunction holding a lambda that captures a Tracked. Inside: the init capture constructs the
    // Tracked directly in the lambda (1 alive), the lambda temporary is moved into the Function's buffer (2 alive),
    // then the temporary is destroyed at the end of the return statement (1 alive)
    const auto makeTrackedFunction = [&callbacks]() -> TrackedFunction {
        return [tracked = ValueType{std::string{"data"}, callbacks}]() {
            return tracked.data.size();
        };
    };

    SECTION("Destroyed with its Function") {
        {
            const TrackedFunction function = makeTrackedFunction();
            CHECK(aliveCount == 1);
            CHECK(function() == 4);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Copies are all destroyed") {
        {
            const TrackedFunction function = makeTrackedFunction();
            const TrackedFunction copy{function};
            CHECK(aliveCount == 2);
            CHECK(copy() == 4);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Moving destroys the source's callable") {
        {
            TrackedFunction function = makeTrackedFunction();
            const TrackedFunction moved{std::move(function)};
            // Unlike Option and Result, a moved-from Function holds nothing, so its moved-from Tracked was destroyed
            CHECK(aliveCount == 1);
            CHECK(function.is_empty());
            CHECK(moved() == 4);
        }
        CHECK(aliveCount == 0);
    }

    SECTION("Assignment destroys the old callable") {
        {
            TrackedFunction function = makeTrackedFunction();
            TrackedFunction other = makeTrackedFunction();
            CHECK(aliveCount == 2);

            // Ours destroyed, a copy of other's constructed
            function = other;
            CHECK(aliveCount == 2);

            // Ours destroyed, other's moved in and other's destroyed
            function = std::move(other);
            CHECK(aliveCount == 1);
            CHECK(other.is_empty());

            // Ours destroyed and replaced with a trivial lambda, which isn't a Tracked
            function = []() {
                return std::size_t{0};
            };
            CHECK(aliveCount == 0);
            CHECK(function() == 0);

            function = makeTrackedFunction();
            CHECK(aliveCount == 1);
            function.reset();
            CHECK(aliveCount == 0);
        }
        CHECK(aliveCount == 0);
    }
}

// The whole point of Function: it never touches the heap. Sealing the heap makes any allocation, from anywhere, call
// the default violation callback, which prints a stacktrace and exits the test process immediately, so an allocation
// here fails loudly rather than silently.
//
// Catch's own macros may allocate (they build strings for their messages), so there's no CHECK or REQUIRE between
// sealing and unsealing, the results are stored in plain locals and checked afterwards
TEST_CASE("Function never allocates") {
    int total = 0;
    int trivialResult = 0;
    int trivialCopyResult = 0;
    int trivialMovedResult = 0;
    int functionPointerResult = 0;
    int nonTrivialResult = 0;
    int nonTrivialCopyResult = 0;
    int nonTrivialMovedResult = 0;
    bool movedFromIsEmpty = false;

    heap::seal_heap();
    {
        // Trivially copyable lambda, capturing an int by value and an int by reference, the memcpy path
        int offset = 100;
        BinaryFunction trivial = [offset, &total](int a, int b) {
            total += a + b;
            return a + b + offset;
        };
        const BinaryFunction trivialCopy{trivial};
        BinaryFunction trivialMoved{std::move(trivial)};
        movedFromIsEmpty = trivial.is_empty();
        trivial = trivialCopy;
        trivialResult = trivial(1, 2);
        trivialCopyResult = trivialCopy(3, 4);
        trivialMovedResult = trivialMoved(5, 6);

        // Function pointer
        trivialMoved = add;
        functionPointerResult = trivialMoved(1, 1);

        // Not trivially copyable lambda, the manage_f path
        pican::Function<int(), SMALL_CAPACITY> nonTrivial = [capture = NotTriviallyCopyable{7}]() {
            return capture.value;
        };
        const pican::Function<int(), SMALL_CAPACITY> nonTrivialCopy{nonTrivial};
        const pican::Function<int(), SMALL_CAPACITY> nonTrivialMoved{std::move(nonTrivial)};
        nonTrivial = nonTrivialCopy;
        nonTrivialResult = nonTrivial();
        nonTrivialCopyResult = nonTrivialCopy();
        nonTrivialMovedResult = nonTrivialMoved();
    }
    heap::unseal_heap();

    // Reaching here at all means nothing allocated, the rest checks that everything also behaved correctly
    CHECK(movedFromIsEmpty);
    CHECK(trivialResult == 103);
    CHECK(trivialCopyResult == 107);
    CHECK(trivialMovedResult == 111);
    CHECK(total == 3 + 7 + 11);
    CHECK(functionPointerResult == 2);
    CHECK(nonTrivialResult == 7);
    CHECK(nonTrivialCopyResult == 7);
    CHECK(nonTrivialMovedResult == 7);
}
