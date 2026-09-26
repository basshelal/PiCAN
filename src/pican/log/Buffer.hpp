#pragma once

#include "pican/log/Entry.hpp"
#include "pican/core/core.hpp"
#include "pican/ds/ds.hpp"

namespace pican::log {

class Buffer {
private:  // fields
    ThreadIdentity threadIdentity_f;
    ds::RingBuffer<Entry> entries_f;

public:  // constructor
    explicit Buffer(const ds::Array<Entry>& array) :
        threadIdentity_f{"Unknown"}, entries_f{array, ds::RingBufferOverflowBehavior::OVERWRITE_OLDEST} {
    }

public:  // lifetime
    Buffer(const Buffer& rhs) = delete;

    Buffer(Buffer&& rhs) noexcept = delete;

    Buffer&
    operator=(const Buffer& rhs) & = delete;

    Buffer&
    operator=(Buffer&& rhs) & noexcept = delete;

    ~Buffer() = default;

public:  // getters
    [[nodiscard]]
    const ThreadIdentity&
    thread_identity() const& {
        return this->threadIdentity_f;
    }

    [[nodiscard]]
    const ds::RingBuffer<Entry>&
    entries() const& {
        return this->entries_f;
    }

public:  // friends
    friend class LoggerThread;
};

}  // namespace pican::log
