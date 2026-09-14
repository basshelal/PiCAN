module;

#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <utility>

#include <pthread.h>
#include <unistd.h>

export module pican.core:functions;

import :types;
import stacktrace;
import fmt;

export namespace pican {

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

[[noreturn]]
inline void
exit_immediately() {
    std::_Exit(1);
}

[[noreturn]]
inline void
vpanic(fmt::string_view format, fmt::format_args args) {
    std::array<char, 1'024> messageBuffer = {};
    const fmt::format_to_n_result<char*> formattedMessage =
        fmt::vformat_to_n(messageBuffer.data(), messageBuffer.size() - 1, format, args);
    ::write(STDERR_FILENO, messageBuffer.data(), formattedMessage.size);
    ::write(STDERR_FILENO, "\n", 1);
    stacktrace::print_stacktrace(stderr);
    pican::exit_immediately();
}

template<typename... Args_TP>
[[noreturn]]
inline void
panic(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    vpanic(format.get(), fmt::make_format_args(args...));
}

[[noreturn]]
inline void
unreachable() {
    panic("Unreachable!");
}

[[nodiscard]]
Milliseconds
get_current_millis() {
    using namespace std;
    const chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
    const chrono::milliseconds sinceEpoch = chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    return sinceEpoch.count();
}

template<typename... Args_TP>
[[noreturn]]
inline void
todo(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    vpanic(format.get(), fmt::make_format_args(args...));
}

template<typename TP, typename... Args>
constexpr TP*
construct_at(TP* location, Args&&... args) {
    if constexpr (std::is_array_v<TP>) {
        return ::new (std::addressof(*location)) TP[1]();
    } else {
        return ::new (std::addressof(*location)) TP(std::forward<Args>(args)...);
    }
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
