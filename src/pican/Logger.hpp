#pragma once

#include <array>
#include <cstdint>

#include <fmt/format.h>

#include "pican/EventFD.hpp"
#include "pican/RingBuffer.hpp"
#include "pican/Types.hpp"

namespace pican {

class LoggerThread;

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

class LogWriter {
private:  // fields
    std::string_view name_f;
    Level level_f;
    FileDescriptor fileDescriptor_f;

public:  // constructor
    LogWriter(const std::string_view& name, Level level, FileDescriptor fileDescriptor) :
        name_f{name}, level_f{level}, fileDescriptor_f{fileDescriptor} {
    }

public:  // copy-control
    LogWriter(const LogWriter& rhs) = default;

    LogWriter(LogWriter&& rhs) noexcept = default;

    LogWriter&
    operator=(const LogWriter& rhs) = default;

    LogWriter&
    operator=(LogWriter&& rhs) noexcept = default;

    ~LogWriter() = default;

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

class Logger {
private:  // types
    static constexpr SizeBytes ACTUAL_MESSAGE_ENTRY_SIZE =
        MESSAGE_MAX_SIZE + 2;  // + 1 for truncation symbol and + 1 for null byte

    struct Entry {
        Level level;
        Milliseconds timestamp;
        ThreadId threadId;
        std::array<char, ACTUAL_MESSAGE_ENTRY_SIZE> message;
    };

private:  // fields
    RingBuffer<Entry> entries_f;
    Array<LogWriter> writers_f;
    Level level_f;
    Count writersCount_f;
    EventFD eventfd_f;

private:  // constructor
    Logger(const Array<Entry>& entries, const Array<LogWriter>& writers, Level level) :
        entries_f{entries, RingBufferOverflowBehavior::OVERWRITE_OLDEST}, writers_f{writers}, level_f{level},
        writersCount_f{0}, eventfd_f{EventFD::Mode::NOTIFY} {
    }

private:
    using This = Logger;
    static bool initialized_sf;
    static Logger* instance_sf;

public:  // member functions
    static bool
    initialize(Count maxEntriesCount, Count maxWritersCount, Level level);

    static inline void
    set_level(Level level) {
        This::ensure_initialized();
        This::instance_sf->level_f = level;
    }

    static void
    register_log_writer(const LogWriter& logWriter);

    template<typename... Args_TP>
    static inline void
    log(Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args) {
        This::ensure_initialized();
        Logger& instance = *This::instance_sf;
        if (level > instance.level_f) {
            return;
        }

        const Milliseconds timestamp = pican::get_current_millis();
        Entry entry{level, timestamp};
        char* messageBuffer = entry.message.data();

        fmt::format_to_n_result<decltype(messageBuffer)> result =
            fmt::format_to_n(messageBuffer, MESSAGE_MAX_SIZE, format, std::forward<Args_TP>(args)...);

        std::size_t writeCount = result.size;
        if (writeCount < MESSAGE_MAX_SIZE) {
            messageBuffer[writeCount] = '\0';
        } else {
            // entry was truncated!
            messageBuffer[MESSAGE_MAX_SIZE] = MESSAGE_TRUNCATED_CHAR;
            messageBuffer[entry.message.size() - 1] = NULL_TERMINATOR_CHAR;
        }

        instance.entries_f.push_copy(entry);
        instance.eventfd_f.notify();
    }

    template<typename... Args_TP>
    static inline void
    error(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
        This::log(Level::ERROR, format, args...);
    }

    template<typename... Args_TP>
    static inline void
    warn(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
        This::log(Level::WARN, format, args...);
    }

private:  // member functions
    static inline void
    ensure_initialized() {
        if (!This::initialized_sf) {
            pican::panic("Logger not initialized!");
        }
    }

public:  // friends
    friend class pican::LoggerThread;
};

}  // namespace pican
