#pragma once

#include <cstdint>

#include "pican/Array.hpp"
#include "pican/ArrayList.hpp"
#include "pican/IApplicationThread.hpp"
#include "pican/Thread.hpp"
#include "pican/log/Buffer.hpp"
#include "pican/log/Sink.hpp"

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
    Array<log::Buffer> buffers_f;
    Count buffersCount_f;
    ArrayList<log::Sink> sinks_f;
    EventFD eventfd_f;
    Thread thread_f;
    ThreadCounter counter_f;
    CopyableAtomic<bool> isRunning_f;

private:  // constructor
    LoggerThread(log::Level level, ThreadName name, Array<log::Buffer> buffers, Array<log::Sink> sinks);

public:  // lifetime
    LoggerThread(const LoggerThread& rhs) = delete;

    LoggerThread(LoggerThread&& rhs) noexcept = default;

    LoggerThread&
    operator=(const LoggerThread& rhs) & = delete;

    LoggerThread&
    operator=(LoggerThread&& rhs) & noexcept = default;

    ~LoggerThread() override = default;

public:  // member functions
    // clang-format off
    virtual ThreadState
    start() & override;

    virtual ThreadState
    stop() & override;
    // clang-format on

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& override;

    [[nodiscard]]
    virtual ThreadCounterValue
    thread_counter_value() const& override;

    [[nodiscard]]
    virtual const ThreadIdentity&
    thread_identity() const& override;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& override;

    [[nodiscard]]
    LoggerThread::Result
    register_sink(const Sink& sink) &;

    [[nodiscard]]
    LoggerThread::Result
    register_thread(const ThreadIdentity& threadIdentity) &;

    void
    log_entry(const Entry& entry) &;

public:  // static functions
    [[nodiscard]]
    static pican::Result<LoggerThread*, Error>
    create(mem::Block block, log::Level level, ThreadName name, Count sinkCount, Count bufferEntryCount);

private:  // member functions
    [[nodiscard]]
    Buffer*
    get_buffer_of_thread(const ThreadId& id) const&;

private:  // static functions
    static void
    runnable(LoggerThread* self);
};

}  // namespace pican::log
