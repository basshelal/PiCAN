#pragma once

#include <atomic>
#include <cstdint>

#include <sys/eventfd.h>
#include <unistd.h>

#include "pican/Utils.hpp"

namespace pican {
class EventFD {
public:  // types
    enum class Mode : std::uint8_t {
        NOTIFY,
        COUNTER,
    };

private:  // fields
    int fd_f;
    Mode mode_f;

public:  // constructors
    EventFD(Mode mode);

public:  // copy-control
    EventFD(const EventFD& rhs) = delete;

    EventFD(EventFD&& rhs) noexcept = delete;

    EventFD&
    operator=(const EventFD& rhs) & = delete;

    EventFD&
    operator=(EventFD&& rhs) & noexcept = delete;

    ~EventFD();

public:  // functions
    void
    notify() const&;

    std::uint64_t
    wait_blocking() const&;

    void
    close() &;

public:  // getters
    [[nodiscard]]
    inline bool
    is_open() const& {
        return this->fd_f != -1;
    }
};
}  // namespace pican
