#pragma once

#include <array>
#include <cstdint>

#include <fmt/format.h>

#include "pican/EventFD.hpp"
#include "pican/RingBuffer.hpp"
#include "pican/Types.hpp"
#include "pican/log/Entry.hpp"
#include "pican/log/Utils.hpp"

namespace pican::log {

class LoggerThread;

class Logger {
private:  // fields
    std::string_view name_f;
    Level level_f;
    FileDescriptor fileDescriptor_f;

public:  // constructor
    Logger(const std::string_view& name, Level level, FileDescriptor fileDescriptor) :
        name_f{name}, level_f{level}, fileDescriptor_f{fileDescriptor} {
    }

public:  // copy-control
    Logger(const Logger& rhs) = default;

    Logger(Logger&& rhs) noexcept = default;

    Logger&
    operator=(const Logger& rhs) = default;

    Logger&
    operator=(Logger&& rhs) noexcept = default;

    ~Logger() = default;

public:  // getters
    [[nodiscard]]
    inline std::string_view
    name() const& {
        return this->name_f;
    }

    [[nodiscard]]
    inline Level
    level() const& {
        return this->level_f;
    }

    [[nodiscard]]
    inline FileDescriptor
    file_descriptor() const& {
        return this->fileDescriptor_f;
    }
};

}  // namespace pican::log
