#include "pican/can/CanThread.hpp"

#include <cstring>

#include <fmt/core.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pican/Application.hpp"
#include "pican/Contracts.hpp"
#include "pican/Result.hpp"
#include "pican/log/Buffer.hpp"
#include "pican/log/LoggerThread.hpp"
#include "pican/mem/Manager.hpp"

namespace pican::can {

CanThread::CanThread(InterfaceName interfaceName, Array<Event> uiRingBufferArray, Array<Event> netRingBufferArray) :
    interfaceName_f{interfaceName}, socketFd_f{0}, thread_f{&This::runnable, this}, isRunning_f{false},
    identity_f{0, "CanThread"}, uiEventBuffer_f{uiRingBufferArray, RingBufferOverflowBehavior::OVERWRITE_OLDEST},
    netEventBuffer_f{netRingBufferArray, RingBufferOverflowBehavior::OVERWRITE_OLDEST}, threadCounter_f{0} {
    int socketFd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd < 0) {
        return;
    }
    this->socketFd_f = socketFd;

    int res;

    // Get interface index
    struct ifreq request{};
    std::strncpy(request.ifr_name, interfaceName.data(), IFNAMSIZ - 1);
    res = ::ioctl(this->socketFd_f, SIOCGIFINDEX, &request);
    if (res < 0) {
        ::close(this->socketFd_f);
        this->socketFd_f = -1;
        return;
    }

    // Bind socket
    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = request.ifr_ifindex;

    res = ::bind(this->socketFd_f, (struct sockaddr*) &addr, sizeof(addr));
    if (res < 0) {
        ::close(this->socketFd_f);
        this->socketFd_f = -1;
        return;
    }

    const SizeBytes internalBufferSize = 8'192 * 2;
    // Set socket buffer size
    res = ::setsockopt(this->socketFd_f, SOL_SOCKET, SO_RCVBUF, &internalBufferSize, sizeof(internalBufferSize));
    if (res < 0) {
        ::close(this->socketFd_f);
        this->socketFd_f = -1;
        return;
    }

    // Set Receive Timeout
    // If we don't do this, read() will block forever.
    // We set it to 3 second. If no frame arrives, read() returns -1 with errno=EAGAIN.
    // This gives our thread a chance to check running flag and exit
    struct timeval timeVal{};
    timeVal.tv_sec = 3;
    timeVal.tv_usec = 0;
    res = ::setsockopt(this->socketFd_f, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeVal, sizeof(timeval));
    if (res < 0) {
        ::close(this->socketFd_f);
        this->socketFd_f = -1;
        return;
    }
}

void
CanThread::start() & {
    if (this->thread_f.is_running()) {
        return;
    }
    this->thread_f.start();
}

/* static */
void
CanThread::runnable(CanThread* self) {
    LinuxCanFrame linuxFrame{};
    pican::can::Event event{};
    const ssize_t expectedReadBytes = static_cast<ssize_t>(sizeof(LinuxCanFrame));

    while (self->isRunning_f.load(std::memory_order_relaxed)) {
        // block read until a frame is found or a timeout happens
        const ssize_t readBytes = ::read(self->socketFd_f, &linuxFrame, sizeof(struct can_frame));

        // bad case, no frame read
        if (readBytes < 0) {
            // Did we timeout?
            if (errno == EAGAIN) {
                // check running flag again
                if (!self->isRunning_f.load(std::memory_order_relaxed)) {
                    break;  // exit runnable
                }
            }
            // Socket was closed
            if (errno == EBADF) {
                break;  // exit
            }
            if (self->isRunning_f.load(std::memory_order_relaxed)) {
                // Unknown reason for failure, log
                pican::log_error("Error: {}", ::strerror(errno));
            }
            continue;
        }

        // unexpected read bytes size, probably incomplete frame
        if (readBytes < expectedReadBytes) {
            pican::log_warn("Incomplete CAN frame read");
            continue;
        }
        // happy path
        assert(readBytes == expectedReadBytes);
        event.set_from_linux_can_frame(linuxFrame);

        self->uiEventBuffer_f.push_copy(event);
        self->netEventBuffer_f.push_copy(event);
    }
}

}  // namespace pican::can
