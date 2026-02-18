#pragma once

#include <atomic>
#include <string_view>

#include <pthread.h>
#include <unistd.h>

#include "pican/Types.hpp"

namespace pican {

using ProcessId = pid_t;
using ThreadId = ProcessId;
using ThreadName = std::string_view;
using ThreadCounterValue = std::uint64_t;
using ThreadCounter = std::atomic<ThreadCounterValue>;

struct ThreadIdentity {
    ThreadId id;
    ThreadName name;

public:  // lifetime
    ThreadIdentity(const ThreadIdentity& rhs) = default;

    ThreadIdentity(ThreadIdentity&& rhs) noexcept = default;

    ThreadIdentity&
    operator=(const ThreadIdentity& rhs) & = default;

    ThreadIdentity&
    operator=(ThreadIdentity&& rhs) & noexcept = default;

    ~ThreadIdentity() = default;

public:  // operators
    bool
    operator==(const ThreadIdentity& rhs) const {
        return this->id == rhs.id && this->name == rhs.name;
    }
};

enum class ThreadState : std::uint8_t {
    CREATED,
    RUNNING,
    STOPPED,
};

class Thread {
public:  // types
    template<typename CallableArg_TP>
    using Callable = void (*)(CallableArg_TP* arg);

    using CallableNoArg = void (*)();

private:  // types
    using ErasedCallable = void (*)(void* arg);
    using Invoker = void (*)(ErasedCallable callable, void* arg);

public:  // fields
    Invoker invoker_f;
    ErasedCallable callable_f;
    void* callableArg_f;
    std::atomic<ThreadState> state_f;
    pthread_t pthread_f;
    pthread_attr_t pthreadAttr_f;
    ThreadId threadId_f;

public:  // constructors
    template<typename CallableArg_TP>
    Thread(const Callable<CallableArg_TP>& callable, CallableArg_TP* arg) :
        callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{arg}, state_f{ThreadState::CREATED},
        pthread_f{0}, pthreadAttr_f{}, threadId_f{0} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            Callable<CallableArg_TP> castCallable = reinterpret_cast<Callable<CallableArg_TP>>(erasedCallable);
            CallableArg_TP* castArg = reinterpret_cast<CallableArg_TP*>(erasedArg);
            castCallable(castArg);
        };
    }

    Thread(const CallableNoArg& callable) :
        callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{nullptr}, state_f{ThreadState::CREATED},
        pthread_f{0}, pthreadAttr_f{}, threadId_f{0} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            CallableNoArg castCallable = reinterpret_cast<CallableNoArg>(erasedCallable);
            castCallable();
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
    ThreadState
    state() const& {
        return this->state_f.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    bool
    is_running() const& {
        return this->state() == ThreadState::RUNNING;
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
    calling_thread_id();

    [[nodiscard]]
    static ThreadId
    main_thread_id();
};

}  // namespace pican
