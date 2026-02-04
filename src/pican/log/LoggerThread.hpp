#pragma once

#include "Logger.hpp"
#include "pican/Array.hpp"
#include "pican/Thread.hpp"
#include "pican/ThreadManager.hpp"
#include "pican/log/Buffer.hpp"

namespace pican {
class ThreadManager;
}

namespace pican::log {

class LoggerThread {
private:  // types
    using This = LoggerThread;

private:  // static fields
    static LoggerThread* instance_sf;

private:  // fields
    Level level_f;
    Array<Buffer> buffers_f;
    Count buffersCount_f;
    Array<Logger> loggers_f;
    Count loggersCount_f;
    EventFD eventfd_f;
    Thread thread_f;

private:  // constructor
    LoggerThread(Level level, Count maxBuffersCount, Count maxLoggersCount);

public:  // member functions
    static void
    initialize(Level level, Count maxBuffersCount, Count maxLoggersCount);

    static void
    register_logger(const Logger& logger);

    static void
    register_thread(ThreadId threadId, Count bufferEntryCount);

    static void
    start_thread();

    template<typename... Args_TP>
    static inline void
    log(Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args) {
        This::ensure_initialized();
        LoggerThread& instance = *This::instance_sf;
        if (level > instance.level_f) {
            return;
        }

        const ThreadId currentThreadId = Thread::calling_thread();

        Buffer* foundBuffer = instance.get_buffer_of_thread(currentThreadId);
        if (foundBuffer == nullptr) {
            return;
            // no buffer for the calling thread
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

        foundBuffer->entries().push_copy(entry);
        instance.eventfd_f.notify();
    }

private:  // member functions
    [[nodiscard]]
    Buffer*
    get_buffer_of_thread(ThreadId threadId) const&;

private:  // static functions
    static void
    ensure_initialized();

    static void
    runnable(LoggerThread* self);

public:  // friends
    friend class pican::ThreadManager;
};
}  // namespace pican::log
