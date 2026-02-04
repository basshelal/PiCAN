#pragma once

#include "pican/Types.hpp"

namespace pican::log {

constexpr SizeBytes MESSAGE_MAX_SIZE = static_cast<SizeBytes>(256);
constexpr char MESSAGE_TRUNCATED_CHAR = '|';
constexpr char NULL_TERMINATOR_CHAR = '\0';

enum class Level : std::uint8_t {
    NONE = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    DEBUG = 4,
    VERBOSE = 5,
};

}  // namespace pican
