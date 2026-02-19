#pragma once

#include <atomic>
#include <cstdint>

#include <sys/eventfd.h>
#include <unistd.h>

#include "pican/Utils.hpp"

namespace pican {
class EventFD {
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
    explicit EventFD(Mode mode);

public:  // lifetime
    EventFD(const EventFD& rhs) = delete;

    EventFD(EventFD&& rhs) noexcept;

    EventFD&
    operator=(const EventFD& rhs) & = delete;

    EventFD&
    operator=(EventFD&& rhs) & noexcept;

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
        return this->fd_f != NULL_FD;
    }
};
}  // namespace pican
