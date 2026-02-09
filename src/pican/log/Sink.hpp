#pragma once

#include <array>
#include <cstdint>
#include <cstdio>

#include <fmt/format.h>

#include "pican/EventFD.hpp"
#include "pican/RingBuffer.hpp"
#include "pican/Types.hpp"
#include "pican/File.hpp"
#include "pican/log/Entry.hpp"
#include "pican/log/Utils.hpp"

namespace pican::log {

class LoggerThread;

class Sink {
private:  // fields
    std::string_view name_f;
    Level level_f;
    File file_f;

public:  // constructor
    Sink(const std::string_view& name, Level level, const File& file);

public:  // copy-control
    Sink(const Sink& rhs) = default;

    Sink(Sink&& rhs) noexcept = default;

    Sink&
    operator=(const Sink& rhs) = default;

    Sink&
    operator=(Sink&& rhs) noexcept = default;

    ~Sink();

public:  // getters
    [[nodiscard]]
    inline const std::string_view&
    name() const& {
        return this->name_f;
    }

    [[nodiscard]]
    inline const Level&
    level() const& {
        return this->level_f;
    }

    [[nodiscard]]
    inline const File&
    file() const& {
        return this->file_f;
    }

public: // friends
    friend class LoggerThread;
};

}  // namespace pican::log
