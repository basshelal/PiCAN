#pragma once

namespace pican::test_utils {

template<typename T>
inline void
do_not_optimize(const T& val) {
    // "r,m" guarantees it works for simple pointers (registers) and complex structs (memory) across both x86 and
    // aarch64
    asm volatile("" : : "r,m"(val) : "memory");
}

}  // namespace pican::test_utils
