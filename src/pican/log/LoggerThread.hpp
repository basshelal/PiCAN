#pragma once

#include <fmt/format.h>

#include "Sink.hpp"
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
    Array<Sink> sinks_f;
    Count sinksCount_f;
    EventFD eventfd_f;
    Thread thread_f;

private:  // constructor
    LoggerThread(Level level, Count threadCount, Count sinkCount, Count threadBufferSize);

public:  // member functions
    static void
    initialize(Level level, Count threadCount, Count sinkCount, Count threadBufferSize);

    static void
    register_logger(const Sink& logger);

    static void
    register_thread(ThreadId id);

    static void
    start_thread();

    // TODO @basshelal Thu 05-Feb-2026 : This needs to be as cheap as possible to call from any thread
    //  possibly hundreds of times a second!
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

        Entry entry{level};
        char* messageBuffer = entry.message_f.data();

        fmt::format_to_n_result<decltype(messageBuffer)> result =
            fmt::format_to_n(messageBuffer, MESSAGE_MAX_SIZE, format, std::forward<Args_TP>(args)...);

        std::size_t writeCount = result.size;
        if (writeCount < MESSAGE_MAX_SIZE) {
            messageBuffer[writeCount] = '\0';
        } else {
            // entry was truncated!
            messageBuffer[MESSAGE_MAX_SIZE] = MESSAGE_TRUNCATED_CHAR;
            messageBuffer[entry.message_f.size() - 1] = NULL_TERMINATOR_CHAR;
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
