#pragma once

#include <atomic>
#include <string_view>

#include <pican/Types.hpp>
#include <pthread.h>
#include <unistd.h>

namespace pican {

// TODO @basshelal Thu 05-Feb-2026 : ThreadId is the pthread id which is basically just the pthread_t,
//  useless except with pthread function contexts, KernelThreadId is more generally useful and should
//  be renamed to ThreadId, this will actually give us the Linux thread id but can only be obtained by
//  being called from the thread itself, using that, we should also allow for thread lookup by id or name
//  and get one from the other
using ProcessId = pid_t;
using ThreadId = ProcessId;
using ThreadName = std::string_view;

class Thread {
public:  // types
    template<typename CallableArg_TP>
    using Callable = void (*)(CallableArg_TP* arg);

    enum class State : std::uint8_t {
        CREATED,
        RUNNING,
        STOPPED,
    };

private:  // types
    using ErasedCallable = void (*)(void* arg);
    using Invoker = void (*)(ErasedCallable callable, void* arg);

public:  // fields
    ThreadName name_f;
    Invoker invoker_f;
    ErasedCallable callable_f;
    void* callableArg_f;
    std::atomic<State> state_f;
    pthread_t pthread_f;
    pthread_attr_t pthreadAttr_f;
    ThreadId threadId_f;

public:  // constructors
    template<typename CallableArg_TP>
    Thread(const ThreadName& name, const Callable<CallableArg_TP>& callable, CallableArg_TP* arg) :
        name_f{name}, callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{arg},
        state_f{State::CREATED}, pthread_f{0}, pthreadAttr_f{}, threadId_f{0} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            Callable<CallableArg_TP> castCallable = reinterpret_cast<Callable<CallableArg_TP>>(erasedCallable);
            CallableArg_TP* castArg = reinterpret_cast<CallableArg_TP*>(erasedArg);
            castCallable(castArg);
        };
    }

public:  // copy-control
    Thread(const Thread& rhs) = delete;

    Thread(Thread&& rhs) noexcept = delete;

    Thread&
    operator=(const Thread& rhs) = delete;

    Thread&
    operator=(Thread&& rhs) noexcept = delete;

    ~Thread();

public:  // getters
    [[nodiscard]]
    const ThreadName&
    name() const& {
        return this->name_f;
    }

    [[nodiscard]]
    State
    state() const& {
        return this->state_f.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    bool
    is_running() const& {
        return this->state() == State::RUNNING;
    }

    [[nodiscard]]
    ThreadId
    id() const& {
        return this->threadId_f;
    }

public:  // member functions
    void
    start();

    void
    stop();

private:  // member functions
    static void*
    pthread_runnable(void* arg);

public:  // static functions
    [[nodiscard]]
    static ThreadId
    calling_thread();
};

}  // namespace pican
