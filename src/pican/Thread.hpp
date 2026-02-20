#pragma once

#include <atomic>
#include <string_view>

#include <pthread.h>
#include <unistd.h>

#include "pican/CopyableAtomic.hpp"
#include "pican/Types.hpp"

namespace pican {

using ProcessId = pid_t;
using ThreadId = ProcessId;
using ThreadName = std::string_view;
using ThreadCounterValue = std::uint64_t;
using ThreadCounter = CopyableAtomic<ThreadCounterValue>;

class ThreadIdentity {
public:  // fields
    ThreadName name;
    ThreadId id;

public:  // constructors
    ThreadIdentity(ThreadName name, ThreadId id) : name{name}, id{id} {
    }

    explicit ThreadIdentity(ThreadName name) : ThreadIdentity{name, 0} {
    }

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
    CopyableAtomic<ThreadState> state_f;
    pthread_t pthread_f;
    pthread_attr_t pthreadAttr_f;
    ThreadIdentity identity_f;

public:  // constructors
    template<typename CallableArg_TP>
    Thread(ThreadName name, const Callable<CallableArg_TP>& callable, CallableArg_TP* arg) :
        callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{arg}, state_f{ThreadState::CREATED},
        pthread_f{0}, pthreadAttr_f{}, identity_f{name} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            Callable<CallableArg_TP> castCallable = reinterpret_cast<Callable<CallableArg_TP>>(erasedCallable);
            CallableArg_TP* castArg = reinterpret_cast<CallableArg_TP*>(erasedArg);
            castCallable(castArg);
        };
    }

    Thread(ThreadName name, const CallableNoArg& callable);

public:  // copy-control
    Thread(const Thread& rhs) = delete;

    Thread(Thread&& rhs) noexcept = default;

    Thread&
    operator=(const Thread& rhs) = delete;

    Thread&
    operator=(Thread&& rhs) noexcept = default;

    ~Thread();

public:  // getters
    [[nodiscard]]
    ThreadState
    state() const&;

    [[nodiscard]]
    bool
    is_running() const&;

    [[nodiscard]]
    const ThreadIdentity&
    thread_identity() const&;

public:  // member functions
    void
    start() &;

    void
    stop() &;

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
