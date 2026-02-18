#pragma once

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include <unistd.h>

#include "pican/Types.hpp"
#include "stacktrace/StackTrace.hpp"

#define INLINE_FLATTEN __attribute((always_inline, flatten))
#define STATIC_FAIL(Message) static_assert(false, Message)
#define PREPROCESSOR_BLOCK(...) \
    do {                        \
        __VA_ARGS__             \
    } while (false)

namespace pican {

template<typename TP>
constexpr TP
clamp(const TP& min, const TP& val, const TP& max) {
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

inline void
write_line(FILE* file, const std::string_view& message) {
    FileDescriptor fileDescriptor = ::fileno(file);
    if (fileDescriptor == -1) {
        return;
    }
    // TODO @basshelal Tue 03-Feb-2026 : Optimize into 1 syscall somehow if possible!
    ::write(fileDescriptor, message.data(), message.length());
    ::write(fileDescriptor, "\n", 1);
}

[[noreturn]]
inline void
exit_immediately() {
    std::_Exit(1);
}

// TODO @basshelal Tue 03-Feb-2026 : Allow for fmt formatting here maybe?
[[noreturn]]
inline void
panic(const std::string_view& message) {
    pican::write_line(stderr, message);
    stacktrace::print_stacktrace(stderr);
    pican::exit_immediately();
}

[[nodiscard]]
Milliseconds
get_current_millis();

[[noreturn]]
inline void
todo(const std::string_view& message) {
    pican::panic(message);
}

constexpr std::uint64_t FNV_OFFSET = 2'166'136'261u;
constexpr std::uint64_t FNV_PRIME = 16'777'619u;

// use Fowler-Noll-Vo (FNV-1a) hashing algorithm
inline std::uint64_t
fnv1a_bytes(const void* data, std::size_t len) {
    const std::uint8_t* ptr = static_cast<const std::uint8_t*>(data);
    std::uint64_t hash = FNV_OFFSET;

    for (std::size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

template<typename TP>
Index
hash(const TP& val) {
    // Check if it's safe to hash raw bytes (Standard Layout)
    static_assert(std::is_standard_layout<TP>::value, "Must be Standard Layout!");

    return pican::fnv1a_bytes(&val, sizeof(TP));
}

}  // namespace pican

#define TODO(Message) pican::todo(Message)

#define TODO_NOT_IMPLEMENTED() pican::todo("Not Implemented")

#define SANITY_CHECK(condition) assert(condition)
