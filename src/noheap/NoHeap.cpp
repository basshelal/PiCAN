#include "noheap/NoHeap.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string_view>

#include <unistd.h>

#include "stacktrace/StackTrace.hpp"

namespace {
alignas(64) static std::atomic<bool> heapSealed_g{false};

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
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return __real_malloc(size);
}

void
__wrap_free(void* ptr) {
    __real_free(ptr);
}

void*
__wrap_calloc(size_t nmemb, size_t size) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return __real_calloc(nmemb, size);
}

void*
__wrap_realloc(void* ptr, size_t size) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return __real_realloc(ptr, size);
}
}

void*
operator new(std::size_t size) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    void* p = std::malloc(size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void*
operator new[](std::size_t size) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    void* p = std::malloc(size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void*
operator new(std::size_t size, std::align_val_t align) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void*
operator new[](std::size_t size, std::align_val_t align) {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void*
operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return std::malloc(size);
}

void*
operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return std::malloc(size);
}

void*
operator new(std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
    return std::aligned_alloc(static_cast<size_t>(align), size);
}

void*
operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    if (heapSealed_g.load(std::memory_order_relaxed)) {
        print_stack_trace_and_exit();
    }
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

namespace noheap {
void
seal_heap() {
    heapSealed_g.store(true, std::memory_order_release);
}

bool
heap_is_sealed() {
    return heapSealed_g.load(std::memory_order_relaxed);
}

}  // namespace noheap
