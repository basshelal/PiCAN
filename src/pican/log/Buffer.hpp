#pragma once

#include "pican/RingBuffer.hpp"
#include "pican/Thread.hpp"
#include "pican/log/Entry.hpp"

namespace pican::log {

class Buffer {
private:  // fields
    ThreadId threadId_f;
    RingBuffer<Entry> entries_f;

public:  // constructor
    Buffer(const ThreadId& threadId, Count entriesCount);

public:  // getters
    [[nodiscard]]
    ThreadId
    thread_id() const& {
        return this->threadId_f;
    }

    [[nodiscard]]
    RingBuffer<Entry>&
    entries() & {
        return this->entries_f;
    }
};

}  // namespace pican::log
