#include "pican/EventFD.hpp"

namespace pican {

EventFD::EventFD(Mode mode) : fd_f{-1}, mode_f{mode} {
    int flags = EFD_CLOEXEC;
    if (mode == Mode::COUNTER) {
        flags |= EFD_SEMAPHORE;
    }
    int ret = ::eventfd(0, flags);
    if (ret == -1) {
        pican::panic("Failed to create eventfd");
    }
    this->fd_f = ret;
}

EventFD::~EventFD() {
    this->close();
}

void
EventFD::notify() const& {
    const std::uint64_t value = 1;
    const ssize_t bytesWritten = ::write(this->fd_f, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        TODO_NOT_IMPLEMENTED();
    }
}

std::uint64_t
EventFD::wait_blocking() const& {
    std::uint64_t value;
    ssize_t bytesRead = ::read(this->fd_f, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        TODO_NOT_IMPLEMENTED();
    }
    return value;
}

void
EventFD::close() & {
    if (!this->is_open()) {
        return;
    }
    int ret = ::close(this->fd_f);
    if (ret == 0) {
        this->fd_f = -1;
    }
}

}  // namespace pican
