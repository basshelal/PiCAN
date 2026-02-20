#pragma once

#include "pican/Thread.hpp"

namespace pican {

/* interface */
class IApplicationThread {
protected:  // lifetime
    IApplicationThread() = default;

    virtual ~IApplicationThread() = default;

public:  // virtual member functions
    virtual ThreadState
    start() & = 0;

    virtual ThreadState
    stop() & = 0;

    [[nodiscard]]
    virtual const ThreadIdentity&
    thread_identity() const& = 0;

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& = 0;

    [[nodiscard]]
    virtual ThreadCounterValue
    thread_counter_value() const& = 0;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& = 0;
};

}  // namespace pican
