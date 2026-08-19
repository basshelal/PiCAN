module;

#include <array>
#include <cstdint>
#include <cstdio>
#include <string_view>

export module pican.log:Sink;

import :utils;
import :Entry;
import :Buffer;
import pican.core;
import pican.fs;
import fmt;

export namespace pican::log {

class Sink {
public:  // types
    enum class Error : std::uint8_t {
        FILE_NOT_FOUND,
        CANNOT_OPEN_FILE,
    };

private:  // fields
    std::string_view name_f;
    LogLevel level_f;
    fs::File file_f;

private:  // constructor
    Sink(const std::string_view& name, LogLevel level, fs::File&& file) :
        name_f{name}, level_f{level}, file_f{std::move(file)} {
    }

public:  // lifetime
    Sink() = delete;

    Sink(const Sink& rhs) = delete;

    Sink(Sink&& rhs) noexcept = default;

    Sink&
    operator=(const Sink& rhs) & = delete;

    Sink&
    operator=(Sink&& rhs) & noexcept = default;

    ~Sink() = default;

public:  // getters
    [[nodiscard]]
    const std::string_view&
    name() const& {
        return this->name_f;
    }

    [[nodiscard]]
    const LogLevel&
    level() const& {
        return this->level_f;
    }

    [[nodiscard]]
    const fs::File&
    file() const& {
        return this->file_f;
    }

public:  // static functions
    [[nodiscard]]
    static pican::Result<Sink, Sink::Error>
    create(std::string_view name, LogLevel level, FilePath path) {
        if (!fs::File::exists(path)) {
            return pican::Result<Sink, Sink::Error>::failure_by_copy(Sink::Error::FILE_NOT_FOUND);
        }
        fs::File file{path};
        const fs::File::SimpleResult openResult = file.open(fs::FileMode::WRITE_ONLY, true);
        if (openResult.is_failure()) {
            return pican::Result<Sink, Sink::Error>::failure_by_copy(Sink::Error::CANNOT_OPEN_FILE);
        }

        return pican::Result<Sink, Sink::Error>::success_by_move(Sink{name, level, std::move(file)});
    }

public:  // friends
    friend class LoggerThread;
};

}  // namespace pican::log
