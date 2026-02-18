#pragma once

#include <chrono>

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
    char*
    message_buffer() & {
        return this->message_f.data();
    }

    // friends
    friend class LoggerThread;
};
}  // namespace pican::log
