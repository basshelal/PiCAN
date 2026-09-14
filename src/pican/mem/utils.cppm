module;

#include <cstdint>
#include <memory>
#include <new>

export module pican.mem:utils;

import stacktrace;
import pican.core;

export namespace pican::mem {

// If compiler doesn't support the hardware constant, default to 64
#ifdef __cpp_lib_hardware_interference_size
constexpr std::size_t CACHE_LINE_ALIGNMENT = std::hardware_destructive_interference_size;
#else
constexpr std::size_t CACHE_LINE_ALIGNMENT = 64;
#endif

constexpr Alignment SYSTEM_DEFAULT_ALIGNMENT = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

constexpr SizeBytes UNKNOWN_SIZE = SIZE_MAX;
constexpr SizeBytes NULL_ADDRESS = 0;

template<typename TP>
inline Address
ptr_to_address(TP* ptr) {
    return reinterpret_cast<Address>(ptr);
}

inline Address
ptr_to_address(nullptr_t ptr) {
    return 0;
}

template<typename TP = void>
inline TP*
address_to_ptr(Address address) {
    return reinterpret_cast<TP*>(address);
}

inline bool
address_is_aligned(Address address, Alignment alignment) {
    return (address % alignment) == 0;
}

[[noreturn]]
inline void
panic_out_of_memory() {
    stacktrace::print_stacktrace(stderr);
    pican::exit_immediately();
}

}  // namespace pican::mem
