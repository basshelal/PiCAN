#pragma once

#include <cstdint>

#include <fmt/format.h>

#include "pican/Array.hpp"
#include "pican/ArrayList.hpp"
#include "pican/IApplicationThread.hpp"
#include "pican/Thread.hpp"
#include "pican/log/Buffer.hpp"
#include "pican/log/Sink.hpp"

namespace pican {
class Application;
}

namespace pican::log {

class LoggerThread : public IApplicationThread {
public:  // types
    enum class Error : std::uint8_t {
        ALREADY_REGISTERED,
        CAPACITY_REACHED,
    };

    using Result = pican::SimpleResult<Error>;

private:  // fields
    log::Level level_f;
    ArrayList<log::Buffer> buffers_f;
    ArrayList<log::Sink> sinks_f;
    EventFD eventfd_f;
    Thread thread_f;
    ThreadCounter counter_f;
    ThreadIdentity identity_f;

private:  // constructor
    LoggerThread(log::Level level, Count sinkCount, Count threadCount, Count threadBufferSize, ThreadName name);

public:  // lifetime
    LoggerThread(const LoggerThread& rhs) = delete;

    LoggerThread(LoggerThread&& rhs) noexcept = delete;

    LoggerThread&
    operator=(const LoggerThread& rhs) & = delete;

    LoggerThread&
    operator=(LoggerThread&& rhs) & noexcept = delete;

    ~LoggerThread() override = default;

public:  // member functions
    void
    start() &;

    [[nodiscard]]
    inline virtual ThreadState
    thread_state() const& override {
        return this->thread_f.state();
    }

    [[nodiscard]]
    inline virtual ThreadCounterValue
    thread_counter_value() const& override {
        return this->counter_f.load(std::memory_order_relaxed);
    }

    [[nodiscard]]
    inline virtual const ThreadIdentity&
    thread_identity() const& override {
        return this->identity_f;
    }

    [[nodiscard]]
    LoggerThread::Result
    register_sink(const Sink& sink) &;

    [[nodiscard]]
    LoggerThread::Result
    register_thread(const ThreadIdentity& identity) &;

    template<typename... Args_TP>
    inline void
    log(Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args) & {
        if (level > this->level_f) {
            return;
        }

        const ThreadId currentThreadId = pican::Thread::calling_thread_id();

        Buffer* foundBuffer = this->get_buffer_of_thread(currentThreadId);
        if (foundBuffer == nullptr) {
            return;
            // no buffer for the calling thread
        }

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
            messageBuffer[entry.message_f.size() - 1] = NULL_TERMINATOR_CHAR;
        }

        foundBuffer->entries_f.push_copy(entry);
        this->eventfd_f.notify();
    }

private:  // member functions
    [[nodiscard]]
    Buffer*
    get_buffer_of_thread(const ThreadId& id) const&;

private:  // static functions
    static void
    runnable(LoggerThread* self);

public:  // friends
    friend class pican::Application;
};


}  // namespace pican::log
