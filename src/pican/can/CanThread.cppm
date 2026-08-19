module;

#include <atomic>
#include <cerrno>
#include <cstring>
#include <string_view>

#include <fmt/core.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pican/contracts.hpp"

export module pican.can:CanThread;

import pican.core;
import pican.ds;
import pican.mem;
import pican.time;
import :Event;
import :Frame;
import :FrameBuffer;
import :CanInfo;

namespace pican::can {
namespace {
void
log_linux_can_frame(const LinuxCanFrame& frame) {
    // Below can become extremely stressful on the log thread, be careful
    // pican::log_verbose(
    //     "{:03X} [{}] {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}", frame.can_id, frame.can_dlc,
    //     frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6],
    //     frame.data[7]
    // );
}

void
log_frame(const Frame& frame) {
    const FrameData& data = frame.data();
    pican::log_verbose(
        "{:03X} [{}] {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {}", frame.id(), frame.length(), data[0],
        data[1], data[2], data[3], data[4], data[5], data[6], data[7], frame.timestamp()
    );
}
}  // namespace
}  // namespace pican::can

export namespace pican::can {

using InterfaceName = std::string_view;

using EventBuffer = ds::RingBuffer<Event>;

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

private:  // fields
    InterfaceName interfaceName_f;
    int socketFd_f;
    Thread thread_f;
    CopyableAtomic<bool> isRunning_f;
    EventBuffer uiEventBuffer_f;
    EventBuffer netEventBuffer_f;
    ThreadCounter threadCounter_f;

private:  // constructors
    // TODO @basshelal Sun 22-Feb-2026 : Instead of the ringbuffers being passed in here, use a callback based system
    //  so that each "consumer" receives when a new Event is found and can add it into that consumer's OWN ringbuffer
    //  possibly processesing and filtering it too, this makes code more localized but means consumers get a slice of
    //  the CanThread's time and thus need to obey the rules, also we need to make it clear that the callback
    //  is being run from the CanThread, callbacks need to be registered before the CanThread is started to make things
    //  easy and thread-safe
    CanThread(
        InterfaceName interfaceName, int socketFd, ThreadName threadName, ds::Array<Event> uiRingBufferArray,
        ds::Array<Event> netRingBufferArray
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
    counter_value() const& override;

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
        std::uint8_t timeoutSeconds, ds::Array<Event> uiRingBufferArray, ds::Array<Event> netRingBufferArray
    );
};
}  // namespace pican::can

export namespace pican::can {

CanThread::CanThread(
    InterfaceName interfaceName, int socketFd, ThreadName threadName, ds::Array<Event> uiRingBufferArray,
    ds::Array<Event> netRingBufferArray
) :
    interfaceName_f{interfaceName}, socketFd_f{socketFd}, thread_f{threadName, &This::runnable, this},
    isRunning_f{false}, uiEventBuffer_f{uiRingBufferArray, ds::RingBufferOverflowBehavior::OVERWRITE_OLDEST},
    netEventBuffer_f{netRingBufferArray, ds::RingBufferOverflowBehavior::OVERWRITE_OLDEST}, threadCounter_f{0} {
}

ThreadState
CanThread::start() & {
    if (this->thread_f.is_running()) {
        return this->thread_state();
    }
    this->isRunning_f.store(true, std::memory_order_release);
    this->thread_f.start();
    return this->thread_state();
}

// TODO @basshelal Tue 10-Mar-2026 : Use eventfd for notifying a stop? How can we merge multiple fds since we already
//  have the socketdfd being read
ThreadState
CanThread::stop() & {
    if (!this->thread_f.is_running()) {
        return this->thread_state();
    }
    this->isRunning_f.store(false, std::memory_order_release);
    return this->thread_state();
}

const ThreadIdentity&
CanThread::thread_identity() const& {
    return this->thread_f.identity();
}

ThreadState
CanThread::thread_state() const& {
    return this->thread_f.state();
}

ThreadCounterValue
CanThread::counter_value() const& {
    return this->threadCounter_f.load();
}

const Thread&
CanThread::backing_thread() const& {
    return this->thread_f;
}

const EventBuffer&
CanThread::ui_event_buffer() const& {
    return this->uiEventBuffer_f;
}

const EventBuffer&
CanThread::net_event_buffer() const& {
    return this->netEventBuffer_f;
}

/* static */
void
CanThread::runnable(CanThread* self) {
    LinuxCanFrame linuxFrame{};
    Frame canFrame{};
    can::CanInfo canInfo{};
    time::Timer<time::TimerUnit::NANOSECONDS> timer{};
    const ssize_t expectedReadBytes = static_cast<ssize_t>(sizeof(LinuxCanFrame));

    while (self->isRunning_f.load(std::memory_order_acquire)) {
        self->threadCounter_f.atomic().fetch_add(1, std::memory_order_acq_rel);
        // block read until a frame is found or a timeout happens

        timer.start();
        const ssize_t readBytes = ::read(self->socketFd_f, &linuxFrame, sizeof(struct can_frame));
        decltype(timer)::DurationUnit frameReadWaitTime = timer.stop();

        // pican::log_info("nanos: {}", nanos);
        // pican::log_info("fps: {}", fps);

        if (!self->isRunning_f.load(std::memory_order_acquire)) {
            break;
        }

        // bad case, no frame read
        if (readBytes < 0) {
            const int err = errno;
            switch (err) {
                case EAGAIN: {
                    pican::log_warn("read() timed out");
                    break;
                }
                case EBADF: {
                    pican::log_warn("socket was closed");
                    break;
                }
                default: {
                    const char* error = static_cast<const char*>(::strerror(errno));
                    pican::log_warn("unknown error: {} {}", ::strerrorname_np(err), error);
                    break;
                }
            }
            continue;
        }

        // unexpected read bytes size, probably incomplete frame
        if (readBytes < expectedReadBytes) {
            pican::log_warn("Incomplete CAN frame read");
            continue;
        }
        // happy path
        CONTRACTS_ASSERT(readBytes == expectedReadBytes);

        timer.start();  // processing time

        log_linux_can_frame(linuxFrame);

        canFrame.set_from_linux_can_frame(linuxFrame);

        log_frame(canFrame);

        decltype(timer)::DurationUnit processingTime = timer.stop();

        canInfo.lastFrameWaitTime = frameReadWaitTime;
        canInfo.lastFrameProcessingTime = processingTime;
        canInfo.lastFrameSize = linuxFrame.len;

        // ApplicationState::get().canInfo_f.write(canInfo);
    }
}

/* static */
pican::Result<CanThread*, CanThread::Error>
CanThread::create(
    mem::Block block, InterfaceName interfaceName, ThreadName threadName, SizeBytes linuxBufferSize,
    std::uint8_t timeoutSeconds, ds::Array<Event> uiRingBufferArray, ds::Array<Event> netRingBufferArray
) {
    using Ret = pican::Result<CanThread*, CanThread::Error>;
    CONTRACTS_PRECONDITION(block.size_bytes() >= sizeof(CanThread));

    int socketFd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd < 0) {
        return Ret::failure_by_copy(Error::FAILED_TO_CREATE_SOCKET);
    }

    int res;

    // Get interface index
    struct ifreq request{};
    std::strncpy(request.ifr_name, interfaceName.data(), IFNAMSIZ - 1);
    res = ::ioctl(socketFd, SIOCGIFINDEX, &request);
    if (res < 0) {
        ::close(socketFd);
        return Ret::failure_by_copy(Error::INTERFACE_NOT_FOUND);
    }

    // Bind socket
    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = request.ifr_ifindex;

    res = ::bind(socketFd, (struct sockaddr*) &addr, sizeof(addr));
    if (res < 0) {
        ::close(socketFd);
        return Ret::failure_by_copy(Error::FAILED_TO_BIND_SOCKET);
    }

    // Set socket buffer size
    res = ::setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, &linuxBufferSize, sizeof(linuxBufferSize));
    if (res < 0) {
        ::close(socketFd);
        return Ret::failure_by_copy(Error::FAILED_TO_SET_SOCKET_BUFFER_SIZE);
    }

    // Set Receive Timeout
    // If we don't do this, read() will block forever.
    // If no frame arrives, read() returns -1 with errno = EAGAIN.
    // This gives our thread a chance to check running flag and exit
    struct timeval timeVal{};
    timeVal.tv_sec = timeoutSeconds;
    timeVal.tv_usec = 0;
    res = ::setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeVal, sizeof(timeval));
    if (res < 0) {
        ::close(socketFd);
        return Ret::failure_by_copy(Error::FAILED_TO_SET_SOCKET_RECEIVE_TIMEOUT);
    }

    CanThread* canThread = new (block.address_to_ptr<CanThread>())
        CanThread{interfaceName, socketFd, threadName, uiRingBufferArray, netRingBufferArray};

    CONTRACTS_ASSERT(canThread != nullptr);

    return Ret::success_by_move(std::move(canThread));
}

}  // namespace pican::can
