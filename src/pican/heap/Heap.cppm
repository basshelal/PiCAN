module;

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string_view>

#include <unistd.h>

export module pican.heap:Heap;

import pican.trace;

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
export namespace pican::heap {
using ViolationCallback = void (*)(std::size_t sizeBytes, void* userData);
}  // namespace pican::heap

namespace {

struct CallbackData {
    pican::heap::ViolationCallback callback;
    void* userData;
};

void
default_violation_callback([[maybe_unused]] std::size_t sizeBytes, [[maybe_unused]] void* userData) {
    std::string_view message{"Illegal allocation!\nHeap has been sealed, stacktrace:\n"};
    ::write(STDERR_FILENO, message.data(), message.length());
    pican::trace::print_stacktrace(stderr, 1);

    _exit(1);  // exit immediately
}

alignas(std::hardware_destructive_interference_size) std::atomic<bool> heapSealed_g{false};
alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> allocationsCount_g{0};
alignas(std::hardware_destructive_interference_size) std::atomic<CallbackData> violationCallback_g{
    CallbackData{.callback = &default_violation_callback, .userData = nullptr}
};

[[maybe_unused]]
void*
check_ptr_alloc(void* const ptr) {
    if (ptr == nullptr) {
        std::string_view message{"Failed to allocate!\n stacktrace:\n\n"};
        ::write(STDERR_FILENO, message.data(), message.length());
        pican::trace::print_stacktrace(stderr, 1);

        _exit(1);  // exit immediately
    }
    return ptr;
}

bool
check_heap_is_not_sealed(std::size_t sizeBytes) {
    if (heapSealed_g.load(std::memory_order::seq_cst)) {
        const CallbackData callbackData = violationCallback_g.load(std::memory_order::seq_cst);
        if (callbackData.callback != nullptr) {
            callbackData.callback(sizeBytes, callbackData.userData);
        }
        return false;
    }
    return true;
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
    if (check_heap_is_not_sealed(size)) {
        allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_malloc(size);
    }
    return nullptr;
}

void
__wrap_free(void* ptr) {
    allocationsCount_g.fetch_sub(1, std::memory_order::seq_cst);
    __real_free(ptr);
}

void*
__wrap_calloc(std::size_t nmemb, std::size_t size) {
    if (check_heap_is_not_sealed(size * nmemb)) {
        allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_calloc(nmemb, size);
    }
    return nullptr;
}

void*
__wrap_realloc(void* ptr, std::size_t size) {
    if (check_heap_is_not_sealed(size)) {
        allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_realloc(ptr, size);
    }
    return nullptr;
}

void*
__wrap_aligned_alloc(std::size_t alignment, std::size_t size) {
    if (check_heap_is_not_sealed(size)) {
        allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_aligned_alloc(alignment, size);
    }
    return nullptr;
}
}

extern "C++" {
void*
operator new(std::size_t size) {
    return std::malloc(size);
}

void*
operator new[](std::size_t size) {
    return std::malloc(size);
}

void*
operator new(std::size_t size, std::align_val_t align) {
    return std::aligned_alloc(static_cast<std::size_t>(align), size);
}

void*
operator new[](std::size_t size, std::align_val_t align) {
    return std::aligned_alloc(static_cast<std::size_t>(align), size);
}

void*
operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    return std::malloc(size);
}

void*
operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
    return std::malloc(size);
}

void*
operator new(std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
    return std::aligned_alloc(static_cast<size_t>(align), size);
}

void*
operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept {
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

export namespace pican::heap {
const pican::heap::ViolationCallback DEFAULT_ILLEGAL_HEAP_USAGE_CALLBACK = default_violation_callback;

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
set_violation_callback(pican::heap::ViolationCallback callback, void* userData) {
    CallbackData callbackData{};
    if (callback == nullptr) {
        callbackData.callback = &default_violation_callback;
        callbackData.userData = nullptr;
    } else {
        callbackData.callback = callback;
        callbackData.userData = userData;
    }
    violationCallback_g.store(callbackData, std::memory_order::seq_cst);
}

void
reset_violation_callback() {
    pican::heap::set_violation_callback(nullptr, nullptr);
}

}  // namespace pican::heap
