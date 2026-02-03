#pragma once

#include <cstddef>

namespace test::heap {
void
seal_heap();

void
unseal_heap();

[[nodiscard]]
bool
heap_is_sealed();

[[nodiscard]]
size_t
allocations_count();

class SealGuard {
public:
    inline SealGuard() {
        test::heap::seal_heap();
    }

    inline ~SealGuard() {
        test::heap::unseal_heap();
    }

public:  // copy-control
    SealGuard(const SealGuard& rhs) = delete;

    SealGuard(SealGuard&& rhs) noexcept = delete;

    SealGuard&
    operator=(const SealGuard& rhs) = delete;

    SealGuard&
    operator=(SealGuard&& rhs) noexcept = delete;
};

#define AUTO_HEAP_SEAL() \
    [[maybe_unused]]     \
    test::heap::SealGuard __s;

}  // namespace test::heap
