#pragma once

#include <algorithm>
#include <iostream>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "core/SourceLocation.hpp"
#include "core/Types.hpp"

#define INLINE_FLATTEN __attribute((always_inline, flatten))
#define STATIC_FAIL(Message) static_assert(false, Message)

namespace core {

template<typename... Args_TP>
[[nodiscard]]
inline std::string
format(fmt::format_string<Args_TP...> formatString, Args_TP&&... args) {
    return fmt::format(fmt::runtime(formatString), args...);
}

[[noreturn]]
inline void
panic(const std::string& message) {
    fmt::println(stderr, message);
    std::exit(-1);
}

[[noreturn]]
inline void
todo(const std::string& message, const SourceLocation& sourceLocation) {
    core::panic(fmt::format("{} at {}", message, sourceLocation.format()));
}

[[noreturn]]
inline void
todo(const std::string& message) {
    core::panic(message);
}
}  // namespace core

#define TODO(Message) core::todo(Message, CURRENT_SOURCE_LOCATION)

#define TODO_NOT_IMPLEMENTED() core::todo("Not Implemented", CURRENT_SOURCE_LOCATION)
