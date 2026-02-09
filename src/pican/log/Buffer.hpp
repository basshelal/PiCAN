#pragma once

#include "pican/RingBuffer.hpp"
#include "pican/Thread.hpp"
#include "pican/log/Entry.hpp"

namespace pican::log {

class Buffer {
private:  // fields
    RingBuffer<Entry> entries_f;
    ThreadId threadId_f;

public:  // constructor
    Buffer(const Array<Entry>& array);

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
    public: // friends
    friend class LoggerThread;
};

}  // namespace pican::log
