#pragma once

#include <chrono>

#include "pican/Thread.hpp"
#include "pican/log/Utils.hpp"

namespace pican::log {

// TODO @basshelal Thu 05-Feb-2026 : Needs level, thread id (kernel!), date time (or something that can
//  compute date time, user formatted message, the formatting into the log format and printing
//  will be done by the logger thread to make a log call as cheap as possible from any calling thread
class Entry {
public:  // types
    using Timestamp = std::chrono::system_clock::time_point;

private:  // fields
    Level level_f;
    Timestamp timestamp_f;
    std::array<char, MESSAGE_MAX_SIZE> message_f;

public:  // constructor
    Entry(const Level& level) : level_f{level}, timestamp_f{std::chrono::system_clock::now()}, message_f{} {
    }

public:  // getters
    [[nodiscard]]
    const Level&
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

    // friends
    friend class LoggerThread;
};
}  // namespace pican::log
