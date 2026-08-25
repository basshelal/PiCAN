module;

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string_view>

#include <unistd.h>

#include "stacktrace/StackTrace.hpp"

export module heap;

export namespace heap {
// TODO @basshelal Tue 25-Aug-2026 : Allow the callback to get a stacktrace somehow???
using HeapSealedCallback = void (*)();
}  // namespace heap

namespace {

void
default_heap_sealed_callback() {
    std::string_view message{"Illegal heap usage!\nHeap has been sealed, stacktrace:\n\n"};
    ::write(STDERR_FILENO, message.data(), message.length());
    stacktrace::print_stacktrace(stderr, 1);

    _exit(1);  // exit immediately
}

alignas(std::hardware_destructive_interference_size) std::atomic<bool> heapSealed_g{false};
alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> allocationsCount_g{0};
alignas(std::hardware_destructive_interference_size) std::atomic<heap::HeapSealedCallback> heapSealedCallback_g{
    default_heap_sealed_callback
};

void*
check_ptr_alloc(void* const ptr) {
    if (ptr == nullptr) {
        std::string_view message{"Failed to allocate!\n stacktrace:\n\n"};
        ::write(STDERR_FILENO, message.data(), message.length());
        stacktrace::print_stacktrace(stderr, 1);

        _exit(1);  // exit immediately
    }
    return ptr;
}

void
check_heap_is_sealed() {
    if (heapSealed_g.load(std::memory_order::seq_cst)) {
        const heap::HeapSealedCallback callback = heapSealedCallback_g.load(std::memory_order::seq_cst);
        callback();
    }
}

}  // namespace

extern "C" {
// Forward declarations of the real functions (provided by the linker)
void*
__real_malloc(std::size_t size);
void
__real_free(void* ptr);
void*
__real_calloc(std::size_t nmemb, std::size_t size);
void*
__real_realloc(void* ptr, std::size_t size);

void*
__real_aligned_alloc(std::size_t alignment, std::size_t size);

void*
__wrap_malloc(std::size_t size) {
    check_heap_is_sealed();
    allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
    return __real_malloc(size);
}

void
__wrap_free(void* ptr) {
    allocationsCount_g.fetch_sub(1, std::memory_order::seq_cst);
    __real_free(ptr);
}

void*
__wrap_calloc(size_t nmemb, size_t size) {
    check_heap_is_sealed();
    allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
    return __real_calloc(nmemb, size);
}

void*
__wrap_realloc(void* ptr, size_t size) {
    check_heap_is_sealed();
    return __real_realloc(ptr, size);
}

void*
__wrap_aligned_alloc(std::size_t alignment, std::size_t size) {
    check_heap_is_sealed();
    allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
    return __real_aligned_alloc(alignment, size);
}
}

// TODO @basshelal Tue 25-Aug-2026 : Test! We should check that we even need the heap sealed checks in the below
//  functions, it might not be needed because we did the above stuff!
extern "C++" {
void*
operator new(std::size_t size) {
    check_heap_is_sealed();
    void* p = std::malloc(size);
    return check_ptr_alloc(p);
}

void*
operator new[](std::size_t size) {
    check_heap_is_sealed();
    void* p = std::malloc(size);
    return check_ptr_alloc(p);
}

void*
operator new(std::size_t size, std::align_val_t align) {
    check_heap_is_sealed();
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    return check_ptr_alloc(p);
}

void*
operator new[](std::size_t size, std::align_val_t align) {
    check_heap_is_sealed();
    void* p = std::aligned_alloc(static_cast<size_t>(align), size);
    return check_ptr_alloc(p);
}

void*
operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    check_heap_is_sealed();
    return std::malloc(size);
}

void*
operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
    check_heap_is_sealed();
    return std::malloc(size);
}

void*
operator new(std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    check_heap_is_sealed();
    return std::aligned_alloc(static_cast<size_t>(align), size);
}

void*
operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    check_heap_is_sealed();
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
}

export namespace heap {
const HeapSealedCallback DEFAULT_ILLEGAL_HEAP_USAGE_CALLBACK = default_heap_sealed_callback;

void
seal_heap() {
    heapSealed_g.store(true, std::memory_order::seq_cst);
}

void
unseal_heap() {
    heapSealed_g.store(false, std::memory_order::seq_cst);
}

[[nodiscard]]
bool
heap_is_sealed() {
    return heapSealed_g.load(std::memory_order::seq_cst);
}

[[nodiscard]]
std::size_t
allocations_count() {
    return allocationsCount_g.load(std::memory_order::seq_cst);
}

void
set_heap_sealed_callback(HeapSealedCallback callback) {
    heapSealedCallback_g.store(callback, std::memory_order::seq_cst);
}

}  // namespace heap
