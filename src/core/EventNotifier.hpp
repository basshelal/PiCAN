#pragma once

#include <atomic>

#include <sys/eventfd.h>
#include <unistd.h>

#include "core/Types.hpp"
#include "core/Utils.hpp"

namespace core {
class EventNotifier {
private:  // fields
    int fd_f;
    UInt64 value_f;
    std::atomic_bool isOpen_f;
    std::atomic_bool isActive_f;

public:  // constructors
    explicit EventNotifier() : fd_f{-1}, value_f{0}, isOpen_f{false}, isActive_f{false} {
        int ret = ::eventfd(0, EFD_CLOEXEC);
        if (ret == -1) {
            TODO("Not implemented erroring, need to use a static factory which returns a Result");
        }
        this->fd_f = ret;
        this->isOpen_f.store(true);
        this->isActive_f.store(true);
    }

public:  // copy-control
    EventNotifier(const EventNotifier& rhs) = delete;

    EventNotifier(EventNotifier&& rhs) noexcept = delete;

    EventNotifier&
    operator=(const EventNotifier& rhs) & = delete;

    EventNotifier&
    operator=(EventNotifier&& rhs) & noexcept = delete;

    ~EventNotifier() {
        if (this->is_active()) {
            this->stop();
        }
        this->close();
    }

public:  // functions
    inline void
    notify() const {
        if (!this->isOpen_f.load() || !this->isActive_f.load()) {
            return;
        }
        const UInt64 value = 1;
        SSize bytesWritten = ::write(this->fd_f, &value, sizeof(value));
        if (bytesWritten != sizeof(value)) {
            TODO_NOT_IMPLEMENTED();
        }
    }

    inline void
    wait_blocking() const {
        UInt64 value;
        SSize bytesRead = ::read(this->fd_f, &value, sizeof(value));
        if (bytesRead != sizeof(value)) {
            TODO_NOT_IMPLEMENTED();
        }
    }

    inline bool
    is_active() {
        return this->isActive_f.load();
    }

    inline void
    stop() {
        if (!this->isActive_f.load()) {
            return;
        }
        this->isActive_f.store(false);
        this->notify();
    }

    inline void
    close() {
        if (!this->isOpen_f.load()) {
            return;
        }
        int ret = ::close(this->fd_f);
        if (ret != 0) {
            TODO_NOT_IMPLEMENTED();
        }
        this->isOpen_f.store(false);
    }

public:  // getters
    [[nodiscard]]
    inline UInt64
    value() const {
        return this->value_f;
    }
};
}  // namespace core
