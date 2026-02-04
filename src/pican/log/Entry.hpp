#pragma once

#include "pican/log/Utils.hpp"

namespace pican::log {
class Entry {
private:
    static constexpr SizeBytes ACTUAL_MESSAGE_ENTRY_SIZE =
        MESSAGE_MAX_SIZE + 2;  // + 1 for truncation symbol and + 1 for null byte
public:
    Level level;
    Milliseconds timestamp;
    int threadId;
    std::array<char, ACTUAL_MESSAGE_ENTRY_SIZE> message;
public: // friends
    friend class LoggerThread;
};
}  // namespace pican::log
