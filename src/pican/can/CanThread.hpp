#pragma once

#include <string_view>

#include "pican/IApplicationThread.hpp"
#include "pican/Result.hpp"
#include "pican/RingBuffer.hpp"
#include "pican/Thread.hpp"
#include "pican/can/Event.hpp"
#include "pican/can/Frame.hpp"

namespace pican::can {

using InterfaceName = std::string_view;

using EventBuffer = RingBuffer<Event>;

class CanThread : public IApplicationThread {
private:  // types
    using This = CanThread;

public:  // types
    enum class Error : std::uint8_t {
        UNKNOWN,
        ALREADY_INITIALIZED,
        ALREADY_RUNNING,
        NOT_INITIALIZED,
        FAILED_TO_CREATE_SOCKET,
        INTERFACE_NOT_FOUND,
        FAILED_TO_SET_SOCKET_BUFFER_SIZE,
        FAILED_TO_SET_SOCKET_RECEIVE_TIMEOUT,
        FAILED_TO_SET_INTERFACE_INDEX,
        FAILED_TO_BIND_SOCKET,
    };

    using Result = pican::SimpleResult<Error>;

private:  // fields
    InterfaceName interfaceName_f;
    int socketFd_f;
    Thread thread_f;
    CopyableAtomic<bool> isRunning_f;
    EventBuffer uiEventBuffer_f;
    EventBuffer netEventBuffer_f;
    ThreadCounter threadCounter_f;

private:  // constructors
    CanThread(
        InterfaceName interfaceName, int socketFd, ThreadName threadName, Array<Event> uiRingBufferArray,
        Array<Event> netRingBufferArray
    );

public:  // lifetime
    CanThread(const CanThread& rhs) = delete;

    CanThread(CanThread&& rhs) noexcept = delete;

    CanThread&
    operator=(const CanThread& rhs) & = delete;

    CanThread&
    operator=(CanThread&& rhs) & noexcept = delete;

    virtual ~CanThread() override = default;

public:  // member functions
    // clang-format off
    virtual ThreadState
    start() & override;

    virtual ThreadState
    stop() & override;
    // clang-format on

    [[nodiscard]]
    virtual const ThreadIdentity&
    thread_identity() const& override;

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& override;

    [[nodiscard]]
    virtual ThreadCounterValue
    thread_counter_value() const& override;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& override;

public:  // getters
    [[nodiscard]]
    inline const EventBuffer&
    ui_event_buffer() const&;

    [[nodiscard]]
    inline const EventBuffer&
    net_event_buffer() const&;

private:  // static functions
    static void
    runnable(CanThread* self);

public:  // static functions
    [[nodiscard]]
    static pican::Result<CanThread*, CanThread::Error>
    create(
        mem::Block block, InterfaceName interfaceName, ThreadName threadName, SizeBytes linuxBufferSize,
        std::uint8_t timeoutSeconds, Array<Event> uiRingBufferArray, Array<Event> netRingBufferArray
    );
};
}  // namespace pican::can
