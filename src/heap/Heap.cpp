#include <cstddef>
#include <cstdlib>
#include <new>

#include "heap/Heap.hpp"

// These must live in a .cpp and not in Heap.hpp:
// - The linker's --wrap=malloc (etc) needs exactly one real, non-inline definition of each __wrap_* symbol to redirect
//   calls to
// - Replacement global operator new/delete are not allowed to be declared inline by the standard
// See Heap.hpp for the details of how the heap sealing works

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
    if (heap::_::check_heap_is_not_sealed(size)) {
        heap::_::allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_malloc(size);
    }
    return nullptr;
}

void
__wrap_free(void* ptr) {
    heap::_::allocationsCount_g.fetch_sub(1, std::memory_order::seq_cst);
    __real_free(ptr);
}

void*
__wrap_calloc(std::size_t nmemb, std::size_t size) {
    if (heap::_::check_heap_is_not_sealed(size * nmemb)) {
        heap::_::allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_calloc(nmemb, size);
    }
    return nullptr;
}

void*
__wrap_realloc(void* ptr, std::size_t size) {
    if (heap::_::check_heap_is_not_sealed(size)) {
        heap::_::allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
        return __real_realloc(ptr, size);
    }
    return nullptr;
}

void*
__wrap_aligned_alloc(std::size_t alignment, std::size_t size) {
    if (heap::_::check_heap_is_not_sealed(size)) {
        heap::_::allocationsCount_g.fetch_add(1, std::memory_order::seq_cst);
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
