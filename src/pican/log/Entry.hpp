#pragma once

#include <chrono>

#include <fmt/format.h>

#include "pican/Thread.hpp"
#include "pican/log/Utils.hpp"

namespace pican::log {

class Entry {
public:  // types
    using Timestamp = std::chrono::system_clock::time_point;

private:  // fields
    log::Level level_f;
    Timestamp timestamp_f;
    std::array<char, MESSAGE_MAX_SIZE> message_f;

public:  // constructor
    Entry(const Level& level) : level_f{level}, timestamp_f{std::chrono::system_clock::now()}, message_f{} {
    }

public:  // getters
    [[nodiscard]]
    const log::Level&
    level() const& {
        return this->level_f;
    }

    [[nodiscard]]
    const Timestamp&
    timestamp() const& {
        return this->timestamp_f;
    }

    [[nodiscard]]
    const std::array<char, MESSAGE_MAX_SIZE>&
    message() const& {
        return this->message_f;
    }

    [[nodiscard]]
    const char*
    message_buffer() const& {
        return this->message_f.data();
    }

    [[nodiscard]]
    char*
    message_buffer() & {
        return this->message_f.data();
    }

    // friends
    friend class LoggerThread;
};

template<typename... Args_TP>
[[nodiscard]]
inline Entry
format_to_entry(Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    Entry entry{level};
    char* messageBuffer = entry.message_buffer();

    fmt::format_to_n_result<char*> result =
        fmt::format_to_n(messageBuffer, MESSAGE_MAX_SIZE, format, std::forward<Args_TP>(args)...);

    std::size_t writeCount = result.size;
    if (writeCount < MESSAGE_MAX_SIZE) {
        messageBuffer[writeCount] = '\0';
    } else {
        // entry was truncated!
        messageBuffer[MESSAGE_MAX_SIZE] = MESSAGE_TRUNCATED_CHAR;
        messageBuffer[entry.message().size() - 1] = NULL_TERMINATOR_CHAR;
    }

    return entry;
}
}  // namespace pican::log
