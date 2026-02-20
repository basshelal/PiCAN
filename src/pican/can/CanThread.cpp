#include "pican/can/CanThread.hpp"

#include <cstring>

#include <fmt/core.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pican/Contracts.hpp"
#include "pican/Log.hpp"
#include "pican/Result.hpp"
#include "pican/log/Buffer.hpp"
#include "pican/log/LoggerThread.hpp"
#include "pican/mem/Manager.hpp"

namespace pican::can {

CanThread::CanThread(
    InterfaceName interfaceName, int socketFd, ThreadName threadName, Array<Event> uiRingBufferArray,
    Array<Event> netRingBufferArray
) :
    interfaceName_f{interfaceName}, socketFd_f{socketFd}, thread_f{threadName, &This::runnable, this},
    isRunning_f{false}, uiEventBuffer_f{uiRingBufferArray, RingBufferOverflowBehavior::OVERWRITE_OLDEST},
    netEventBuffer_f{netRingBufferArray, RingBufferOverflowBehavior::OVERWRITE_OLDEST}, threadCounter_f{0} {
}

/* static */
pican::Result<CanThread*, CanThread::Error>
CanThread::create(
    mem::Block block, InterfaceName interfaceName, ThreadName threadName, SizeBytes linuxBufferSize,
    std::uint8_t timeoutSeconds, Array<Event> uiRingBufferArray, Array<Event> netRingBufferArray
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
    // If no frame arrives, read() returns -1 with errno=EAGAIN.
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

ThreadState
CanThread::start() & {
    if (this->thread_f.is_running()) {
        return this->thread_state();
    }
    this->isRunning_f.store(true, std::memory_order_relaxed);
    this->thread_f.start();
    return this->thread_state();
}

ThreadState
CanThread::stop() & {
    if (!this->thread_f.is_running()) {
        return this->thread_state();
    }
    this->isRunning_f.store(false, std::memory_order_relaxed);
    this->thread_f.stop();
    return this->thread_state();
}

const ThreadIdentity&
CanThread::thread_identity() const& {
    return this->thread_f.thread_identity();
}

ThreadState
CanThread::thread_state() const& {
    return this->thread_f.state();
}

ThreadCounterValue
CanThread::thread_counter_value() const& {
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

namespace {
void
log_linux_can_frame(const LinuxCanFrame& frame) {
    // Below can become extremely stressful on the log thread, be careful
    pican::log_verbose(
        "{:03X} [{}] {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}", frame.can_id, frame.can_dlc,
        frame.data[0], frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6],
        frame.data[7]
    );
}
}  // namespace

/* static */
void
CanThread::runnable(CanThread* self) {
    LinuxCanFrame linuxFrame{};
    pican::can::Event event{};
    const ssize_t expectedReadBytes = static_cast<ssize_t>(sizeof(LinuxCanFrame));

    while (self->isRunning_f.load()) {
        self->threadCounter_f.atomic().fetch_add(1);
        // block read until a frame is found or a timeout happens

        const ssize_t readBytes = ::read(self->socketFd_f, &linuxFrame, sizeof(struct can_frame));

        if (!self->isRunning_f.load()) {
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
                    pican::log_warn("unknown error: {} {}", ::strerrorname_np(err), ::strerror(errno));
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

        log_linux_can_frame(linuxFrame);

        event.set_from_linux_can_frame(linuxFrame);

        self->uiEventBuffer_f.push_copy(event);
        self->netEventBuffer_f.push_copy(event);
    }
}

}  // namespace pican::can
