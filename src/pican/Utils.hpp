#pragma once

#include <fmt/core.h>

#include "pican/SourceLocation.hpp"
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

template<typename... Args_TP>
[[nodiscard]]
inline std::string
format(fmt::format_string<Args_TP...> formatString, Args_TP&&... args) {
    return fmt::format(fmt::runtime(formatString), args...);
}

[[noreturn]]
inline void
panic(const std::string_view& message) {
    fmt::println(stderr, message);
    stacktrace::print_stacktrace(stderr);
    std::abort();
}

[[noreturn]]
inline void
exit_immediately() {
    std::_Exit(1);
}

[[nodiscard]]
Milliseconds
get_current_millis();

[[noreturn]]
inline void
todo(const std::string& message, const SourceLocation& sourceLocation) {
    pican::panic(fmt::format("{} at {}", message, sourceLocation.format()));
}

[[noreturn]]
inline void
todo(const std::string& message) {
    pican::panic(message);
}
}  // namespace pican

#define TODO(Message) pican::todo(Message, CURRENT_SOURCE_LOCATION)

#define TODO_NOT_IMPLEMENTED() pican::todo("Not Implemented", CURRENT_SOURCE_LOCATION)
