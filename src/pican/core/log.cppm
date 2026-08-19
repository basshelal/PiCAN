module;

#include <atomic>
#include <cstdint>
#include <string_view>

#include <unistd.h>

export module pican.core:log;

import :types;
import :functions;
import fmt;

export namespace pican {
enum class LogLevel : std::uint8_t {
    NONE = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    VERBOSE = 4,
};

// fmt::format_args holds the type-erased arguments safely.
using LogFunctionPtr = void (*)(LogLevel level, fmt::string_view format, fmt::format_args args);
constexpr SizeBytes MESSAGE_MAX_SIZE = static_cast<SizeBytes>(128);
constexpr char MESSAGE_TRUNCATED_CHAR = '|';
constexpr char NULL_TERMINATOR_CHAR = '\0';

inline constexpr std::string_view
log_level_to_string(const LogLevel& level) {
    switch (level) {
        case LogLevel::NONE:
            return "NONE";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::VERBOSE:
            return "VERBOSE";
    }
    pican::unreachable();
}

constexpr SizeBytes LEVEL_STRING_MAX_LENGTH = log_level_to_string(LogLevel::VERBOSE).size();

template<typename... Args_TP>
using FormatString = fmt::format_string<Args_TP...>;

}  // namespace pican

namespace pican {

void
default_log_function(LogLevel level, fmt::string_view format, fmt::format_args args) {
    std::array<char, MESSAGE_MAX_SIZE> messageBuffer;

    const fmt::format_to_n_result<char*> formattedMessage =
        fmt::vformat_to_n(messageBuffer.data(), messageBuffer.size(), format, args);

    const std::string_view message(messageBuffer.data(), formattedMessage.size);

    std::array<char, LEVEL_STRING_MAX_LENGTH + 1 + MESSAGE_MAX_SIZE + 2> fullBuffer;

    const fmt::format_to_n_result<char*> fullDataToWrite =
        fmt::format_to_n(fullBuffer.data(), fullBuffer.size(), "{:<4.4} {}\n", log_level_to_string(level), message);

    int fd = STDOUT_FILENO;
    if (level == LogLevel::ERROR) {
        fd = STDERR_FILENO;
    }

    ::write(fd, fullBuffer.data(), fullDataToWrite.size);
}

static_assert(std::is_same_v<decltype(&default_log_function), LogFunctionPtr>);

std::atomic<LogFunctionPtr> logFunction_g{&default_log_function};

}  // namespace pican

// must be outside of anonymous namespace

export namespace pican {

void
set_log_function(const LogFunctionPtr& logFunction) {
    if (logFunction == nullptr) {
        logFunction_g.store(&default_log_function, std::memory_order_consume);
    } else {
        logFunction_g.store(logFunction, std::memory_order_consume);
    }
}

template<typename... Args_TP>
void
log_level(LogLevel level, FormatString<Args_TP...> format, Args_TP&&... args) {
    fmt::format_args type_erased_args = fmt::make_format_args(args...);
    const LogFunctionPtr logFunction = logFunction_g.load(std::memory_order_acquire);
    logFunction(level, format.get(), type_erased_args);
}

template<typename... Args_TP>
inline void
log_error(FormatString<Args_TP...> format, Args_TP&&... args) {
    log_level(LogLevel::ERROR, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_warn(FormatString<Args_TP...> format, Args_TP&&... args) {
    log_level(LogLevel::WARN, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
void
log_info(FormatString<Args_TP...> format, Args_TP&&... args) {
    log_level(LogLevel::INFO, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_verbose(FormatString<Args_TP...> format, Args_TP&&... args) {
    log_level(LogLevel::VERBOSE, format, std::forward<Args_TP>(args)...);
}

}  // namespace pican
