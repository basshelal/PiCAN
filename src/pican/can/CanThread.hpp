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
        FAILED_TO_SET_SOCKET_OPTION,
        FAILED_TO_SET_INTERFACE_INDEX,
        FAILED_TO_BIND_SOCKET,
    };

    using Result = pican::SimpleResult<Error>;

private:  // fields
    InterfaceName interfaceName_f;
    int socketFd_f;
    Thread thread_f;
    std::atomic_bool isRunning_f;
    ThreadIdentity identity_f;
    EventBuffer uiEventBuffer_f;
    EventBuffer netEventBuffer_f;
    ThreadCounterValue threadCounter_f;

public:  // constructors
    CanThread(InterfaceName interfaceName, Array<Event> uiRingBufferArray, Array<Event> netRingBufferArray);

public:  // lifetime
    CanThread(const CanThread& rhs) = delete;

    CanThread(CanThread&& rhs) noexcept = delete;

    CanThread&
    operator=(const CanThread& rhs) & = delete;

    CanThread&
    operator=(CanThread&& rhs) & noexcept = delete;

    virtual ~CanThread() override = default;

public:  // member functions
    void
    start() &;

    void
    stop() &;

    [[nodiscard]]
    virtual const ThreadIdentity&
    thread_identity() const& override {
        return this->identity_f;
    }

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& override {
        return this->thread_f.state();
    }

    [[nodiscard]]
    virtual ThreadCounterValue
    thread_counter_value() const& override {
        return this->threadCounter_f;
    }

public:  // getters
    [[nodiscard]]
    inline const EventBuffer&
    ui_event_buffer() const& {
        return this->uiEventBuffer_f;
    }

    [[nodiscard]]
    inline const EventBuffer&
    net_event_buffer() const& {
        return this->netEventBuffer_f;
    }

private:  // static functions
    static void
    runnable(CanThread* self);
};
}  // namespace pican::can
