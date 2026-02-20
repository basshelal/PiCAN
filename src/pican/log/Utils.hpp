#pragma once

#include <string_view>

#include "pican/Types.hpp"
#include "pican/Utils.hpp"

namespace pican::log {

constexpr SizeBytes MESSAGE_MAX_SIZE = static_cast<SizeBytes>(128);
constexpr char MESSAGE_TRUNCATED_CHAR = '|';
constexpr char NULL_TERMINATOR_CHAR = '\0';

enum class Level : std::uint8_t {
    NONE = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    VERBOSE = 4,
};

inline constexpr std::string_view
level_to_string(const Level& level) {
    switch (level) {
        case Level::NONE:
            return "NONE";
        case Level::ERROR:
            return "ERROR";
        case Level::WARN:
            return "WARN";
        case Level::INFO:
            return "INFO";
        case Level::VERBOSE:
            return "VERBOSE";
    }
    pican::panic("Unreachable!");
}

constexpr SizeBytes LEVEL_STRING_MAX_LENGTH = level_to_string(Level::VERBOSE).size();

}  // namespace pican::log
