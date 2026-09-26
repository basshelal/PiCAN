#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string_view>
#include <unistd.h>

#include "stacktrace/Stacktrace.hpp"

/**
 * Simple means of having a locked heap by overloading and overwriting all allocating functions, this works by
 * instructing the linker to wrap the malloc and friends functions using the following instructions in CMakeLists.txt
 * per target:
 * "LINKER:--wrap=malloc"
 * "LINKER:--wrap=free"
 * "LINKER:--wrap=calloc"
 * "LINKER:--wrap=realloc"
 * "LINKER:--wrap=aligned_alloc"
 * which will make all calls to malloc go to __wrap_malloc and the original malloc becomes __real_malloc
 * All of the operator new use this malloc but we overload them (and delete) anyway for safety and completeness and to
 * allow us customize their behavior further should the need arise
 */

namespace heap {
using ViolationCallback = void (*)(std::size_t sizeBytes, void* userData);
}  // namespace heap

// Implementation details, not part of the public API, shared with Heap.cpp
namespace heap::_ {

inline void
default_violation_callback([[maybe_unused]] std::size_t sizeBytes, [[maybe_unused]] void* userData) {
    std::string_view message{"Illegal allocation!\nHeap has been sealed, stacktrace:\n"};
    [[maybe_unused]]
    ssize_t _ = ::write(STDERR_FILENO, message.data(), message.length());
    stacktrace::print_stacktrace(stderr, 1);

    _exit(1);  // exit immediately
}

alignas(std::hardware_destructive_interference_size) inline std::atomic<bool> heapSealed_g{false};
alignas(std::hardware_destructive_interference_size) inline std::atomic<std::size_t> allocationsCount_g{0};
alignas(std::hardware_destructive_interference_size) inline std::atomic<heap::ViolationCallback> violationCallback_g{
    &default_violation_callback
};
alignas(std::hardware_destructive_interference_size) inline std::atomic<void*> violationCallbackData_g{nullptr};

static_assert(decltype(heapSealed_g)::is_always_lock_free);
static_assert(decltype(allocationsCount_g)::is_always_lock_free);
static_assert(decltype(violationCallback_g)::is_always_lock_free);
static_assert(decltype(violationCallbackData_g)::is_always_lock_free);

[[maybe_unused]]
inline void*
check_ptr_alloc(void* const ptr) {
    if (ptr == nullptr) {
        std::string_view message{"Failed to allocate!\n stacktrace:\n\n"};
        [[maybe_unused]]
        ssize_t _ = ::write(STDERR_FILENO, message.data(), message.length());
        stacktrace::print_stacktrace(stderr, 1);

        _exit(1);  // exit immediately
    }
    return ptr;
}

inline bool
check_heap_is_not_sealed(std::size_t sizeBytes) {
    if (heapSealed_g.load(std::memory_order::seq_cst)) {
        const heap::ViolationCallback callback = violationCallback_g.load(std::memory_order::seq_cst);
        if (callback != nullptr) {
            void* callbackData = violationCallbackData_g.load(std::memory_order::seq_cst);
            callback(sizeBytes, callbackData);
        }
        return false;
    }
    return true;
}

}  // namespace heap::_

namespace heap {
inline const heap::ViolationCallback DEFAULT_ILLEGAL_HEAP_USAGE_CALLBACK = _::default_violation_callback;

inline void
seal_heap() {
    _::heapSealed_g.store(true, std::memory_order::seq_cst);
}

inline void
unseal_heap() {
    _::heapSealed_g.store(false, std::memory_order::seq_cst);
}

[[nodiscard]]
inline bool
heap_is_sealed() {
    return _::heapSealed_g.load(std::memory_order::seq_cst);
}

[[nodiscard]]
inline std::size_t
allocations_count() {
    return _::allocationsCount_g.load(std::memory_order::seq_cst);
}

inline void
set_violation_callback(heap::ViolationCallback callback, void* userData) {
    _::violationCallback_g.store(callback, std::memory_order::seq_cst);
    _::violationCallbackData_g.store(userData, std::memory_order::seq_cst);
}

inline void
reset_violation_callback() {
    heap::set_violation_callback(&_::default_violation_callback, nullptr);
}

}  // namespace heap
