#pragma once

#include <atomic>
#include <string_view>

#include <pican/Types.hpp>
#include <pthread.h>
#include <unistd.h>

namespace pican {

using ThreadId = pthread_t;
using ProcessId = pid_t;
using KernelThreadId = ProcessId;

class Thread {
public:  // types
    using Name = std::string_view;

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
    Name name_f;
    Invoker invoker_f;
    ErasedCallable callable_f;
    void* callableArg_f;
    std::atomic<State> state_f;
    pthread_t pthread_f;
    pthread_attr_t pthreadAttr_f;
    KernelThreadId kThreadId_f;

public:  // constructors
    template<typename CallableArg_TP>
    Thread(const Name& name, const Callable<CallableArg_TP>& callable, CallableArg_TP* arg) :
        name_f{name}, callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{arg},
        state_f{State::CREATED}, pthread_f{0}, pthreadAttr_f{}, kThreadId_f{0} {
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
    const Name&
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
        return this->pthread_f;
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
    static inline ThreadId
    calling_thread() {
        return ::pthread_self();
    }
};

}  // namespace pican
