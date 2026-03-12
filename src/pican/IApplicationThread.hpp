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
    thread_identity() const& {
        return this->backing_thread().identity();
    }

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& {
        return this->backing_thread().state();
    };

    [[nodiscard]]
    virtual ThreadCounterValue
    counter_value() const& = 0;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& = 0;
};

}  // namespace pican
