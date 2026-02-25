#include "test/Heap.hpp"

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string_view>

#include <unistd.h>

#include "stacktrace/StackTrace.hpp"

namespace {
alignas(64) static std::atomic<bool> heapSealed_g{false};
alignas(64) static std::atomic<size_t> allocationsCount_g{0};

[[noreturn]]
void
print_stack_trace_and_exit() {
    std::string_view message{"Illegal heap usage!\nHeap has been sealed, stacktrace:\n\n"};
    ::write(STDERR_FILENO, message.data(), message.length());
    stacktrace::print_stacktrace(stderr, 1);

    _exit(1);  // exit immediately
}
}  // namespace

extern "C" {
// Forward declarations of the real functions (provided by the linker)
void*
__real_malloc(size_t size);
void
__real_free(void* ptr);
void*
__real_calloc(size_t nmemb, size_t size);
void*
__real_realloc(void* ptr, size_t size);

void*
__wrap_malloc(size_t size) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return __real_malloc(size);
}

void
__wrap_free(void* ptr) {
    __real_free(ptr);
}

void*
__wrap_calloc(size_t nmemb, size_t size) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return __real_calloc(nmemb, size);
}

void*
__wrap_realloc(void* ptr, size_t size) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return __real_realloc(ptr, size);
}
}

void*
operator new(std::size_t size) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    void* p = std::malloc(size);
    if (p == nullptr) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return p;
}

void*
operator new[](std::size_t size) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    void* p = std::malloc(size);
    if (p == nullptr) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return p;
}

void*
operator new(std::size_t size, std::align_val_t align) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    if (p == nullptr) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return p;
}

void*
operator new[](std::size_t size, std::align_val_t align) {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    if (p == nullptr) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return p;
}

void*
operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return std::malloc(size);
}

void*
operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return std::malloc(size);
}

void*
operator new(std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return std::aligned_alloc(static_cast<size_t>(align), size);
}

void*
operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_seq_cst)) {
        print_stack_trace_and_exit();
    }
    allocationsCount_g++;
    return std::aligned_alloc(static_cast<size_t>(align), size);
}

void
operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void
operator delete(void* ptr, std::align_val_t align) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr, std::align_val_t align) noexcept {
    std::free(ptr);
}

void
operator delete(void* ptr, std::size_t size) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr, std::size_t size) noexcept {
    std::free(ptr);
}

void
operator delete(void* ptr, std::size_t size, std::align_val_t align) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr, std::size_t size, std::align_val_t align) noexcept {
    std::free(ptr);
}

void
operator delete(void* ptr, const std::nothrow_t& tag) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr, const std::nothrow_t& tag) noexcept {
    std::free(ptr);
}

void
operator delete(void* ptr, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    std::free(ptr);
}

void
operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    std::free(ptr);
}

namespace test::heap {
void
seal_heap() {
    heapSealed_g.store(true, std::memory_order_seq_cst);
}

void
unseal_heap() {
    heapSealed_g.store(false, std::memory_order_seq_cst);
}

bool
heap_is_sealed() {
    return heapSealed_g.load(std::memory_order_seq_cst);
}

size_t
allocations_count() {
    return allocationsCount_g.load(std::memory_order_seq_cst);
}
}  // namespace test::heap
