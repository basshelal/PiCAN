#pragma once

#include <array>
#include <cstdint>
#include <cstdio>

#include <fmt/format.h>

#include "pican/EventFD.hpp"
#include "pican/File.hpp"
#include "pican/RingBuffer.hpp"
#include "pican/Types.hpp"
#include "pican/log/Entry.hpp"
#include "pican/log/Utils.hpp"

namespace pican::log {

class LoggerThread;

class Sink {
public:  // types
    enum class Error : std::uint8_t {
        FILE_NOT_FOUND,
        CANNOT_OPEN_FILE,
    };

private:  // fields
    std::string_view name_f;
    Level level_f;
    File file_f;

private:  // constructor
    Sink(const std::string_view& name, Level level, File&&file);

public:  // lifetime
    Sink(const Sink& rhs) = delete;

    Sink(Sink&& rhs) noexcept = default;

    Sink&
    operator=(const Sink& rhs) & = delete;

    Sink&
    operator=(Sink&& rhs) & noexcept = default;

    ~Sink()=default;

public:  // getters
    [[nodiscard]]
    const std::string_view&
    name() const&;

    [[nodiscard]]
    const Level&
    level() const&;

    [[nodiscard]]
    const File&
    file() const&;

public:  // static functions
    [[nodiscard]]
    static pican::Result<Sink, Sink::Error>
    create(std::string_view name, Level level, FilePath path);

public:  // friends
    friend class LoggerThread;
};

}  // namespace pican::log
