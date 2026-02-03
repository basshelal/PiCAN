#include "ReaderThread.hpp"

#include <cstring>

#include <fmt/core.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pican/Contracts.hpp"

namespace pican::can {
void
ReaderThread::start() {
    if (this->isRunning_f.load()) {
        spdlog::warn("CanBusReader already running on {}", this->interfaceName_f);
        return;
    }

    spdlog::info("Starting CanBusReader on {}", this->interfaceName_f);

    // 1. Create socket
    this->socketFd_f = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (this->socketFd_f < 0) {
        spdlog::error("Failed to create CAN socket: {}", std::strerror(errno));
        return;
    }

    // Set receive buffer to 1MB (arbitrary large size)
    int rcvbuf_size = 256 * 1'024;
    if (::setsockopt(this->socketFd_f, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof(rcvbuf_size)) < 0) {
        spdlog::error("Error setting socket buffer size");
    }
    socklen_t optsize;
    ::getsockopt(this->socketFd_f, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, &optsize);

    // 2. Get interface index
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, this->interfaceName_f.c_str(), IFNAMSIZ - 1);
    if (::ioctl(this->socketFd_f, SIOCGIFINDEX, &ifr) < 0) {
        spdlog::error("Failed to get interface index for {}: {}", this->interfaceName_f, std::strerror(errno));
        ::close(this->socketFd_f);
        this->socketFd_f = -1;
        return;
    }

    // 3. Bind socket
    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(this->socketFd_f, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        spdlog::error("Failed to bind CAN socket to {}: {}", interfaceName_f, std::strerror(errno));
        ::close(socketFd_f);
        this->socketFd_f = -1;
        return;
    }

    this->isRunning_f.store(true);
    this->thread_f = std::thread{&ReaderThread::read_loop, this};

    spdlog::info("CanBusReader started successfully on {}", this->interfaceName_f);
}

void
ReaderThread::stop() {
    if (!this->isRunning_f.load()) {
        return;
    }

    spdlog::info("Stopping CanBusReader on {}", this->interfaceName_f);
    this->isRunning_f.store(false);

    // Close socket to unblock read() in the thread (simplest way for blocking IO)
    // Note: In a production system, one might use select/poll/epoll with a pipe for clean shutdown
    if (this->socketFd_f >= 0) {
        ::close(socketFd_f);
        this->socketFd_f = -1;
    }

    if (this->thread_f.joinable()) {
        this->thread_f.join();
    }
    spdlog::info("CanBusReader stopped");
}

void
ReaderThread::read_loop() {
    struct can_frame originalFrame{};
    pican::can::Frame frame{};

    while (this->isRunning_f.load()) {
        // Read CAN frame
        // Note: This is a blocking read. Closing the socket in stop() causes this to return error.
        ssize_t readBytes = ::read(this->socketFd_f, &originalFrame, sizeof(struct can_frame));

        if (readBytes < 0) {
            if (this->isRunning_f.load()) {
                // Only log error if we didn't intentionally stop
                spdlog::error("Error reading from CAN socket: {}", std::strerror(errno));
                // Depending on error, might want to break or continue.
                // For now, if socket is closed/invalid, we break.
                if (errno == EBADF) {
                    break;
                }
            }
            break;
        }

        if (readBytes < static_cast<ssize_t>(sizeof(struct can_frame))) {
            spdlog::warn("Incomplete CAN frame read");
            continue;
        }
        frame.id_f = originalFrame.can_id;
        frame.set_from_linux_can_frame(originalFrame);

        if (this->callback_f) {
            this->callback_f(frame);
        }
    }
}
}  // namespace pican::can
