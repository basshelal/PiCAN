module;

#include <atomic>
#include <cstdint>

#include <sys/eventfd.h>
#include <unistd.h>

#include "pican/macros.hpp"

export module pican.sync:EventFd;

import pican.core;

export namespace pican::sync {
class EventFd {
private:  // constants
    static constexpr int NULL_FD = -1;

public:  // types
    enum class Mode : std::uint8_t {
        NOTIFY,
        COUNTER,
    };

private:  // fields
    int fd_f;
    Mode mode_f;

public:  // constructors
    explicit EventFd(Mode mode) : fd_f{NULL_FD}, mode_f{mode} {
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

public:  // lifetime
    EventFd(const EventFd& rhs) = delete;

    EventFd(EventFd&& rhs) noexcept {
        this->fd_f = rhs.fd_f;
        this->mode_f = rhs.mode_f;
        rhs.fd_f = NULL_FD;
    }

    EventFd&
    operator=(const EventFd& rhs) & = delete;

    EventFd&
    operator=(EventFd&& rhs) & noexcept {
        if (std::addressof(rhs) == this) {
            return *this;
        }
        this->fd_f = rhs.fd_f;
        this->mode_f = rhs.mode_f;
        rhs.fd_f = NULL_FD;
        return *this;
    }

    ~EventFd() {
        this->close();
    }

public:  // functions
    void
    notify() const& {
        const std::uint64_t value = 1;
        const ssize_t bytesWritten = ::write(this->fd_f, &value, sizeof(value));
        if (bytesWritten != sizeof(value)) {
            TODO_NOT_IMPLEMENTED();
        }
    }

    std::uint64_t
    wait_blocking() const& {
        std::uint64_t value;
        ssize_t bytesRead = ::read(this->fd_f, &value, sizeof(value));
        if (bytesRead != sizeof(value)) {
            TODO_NOT_IMPLEMENTED();
        }
        return value;
    }

    void
    close() & {
        if (!this->is_open()) {
            return;
        }
        int ret = ::close(this->fd_f);
        if (ret == 0) {
            this->fd_f = NULL_FD;
        }
    }

public:  // getters
    [[nodiscard]]
    inline bool
    is_open() const& {
        return this->fd_f != NULL_FD;
    }
};
}  // namespace pican::sync
