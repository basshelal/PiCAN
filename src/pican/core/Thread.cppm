module;

#include <atomic>
#include <cstring>
#include <string_view>

#include <pthread.h>
#include <unistd.h>

export module pican.core:Thread;

import :types;
import :CopyableAtomic;

export namespace pican {

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
    FINISHED,
    KILLED,
};

class Thread {
public:  // types
    template<typename CallableArg_TP>
    using Callable = void (*)(CallableArg_TP* arg);

    using CallableNoArg = void (*)();

private:  // types
    using ErasedCallable = void (*)(void* arg);
    using Invoker = void (*)(ErasedCallable callable, void* arg);

private:  // fields
    Invoker invoker_f;
    ErasedCallable callable_f;
    void* callableArg_f;
    CopyableAtomic<ThreadState> state_f;
    pthread_t pthread_f;
    pthread_attr_t pthreadAttr_f;
    ThreadIdentity identity_f;
    CopyableAtomic<bool> runnableInitialized_f;

public:  // constructors
    template<typename CallableArg_TP>
    Thread(ThreadName name, const Callable<CallableArg_TP>& callable, CallableArg_TP* arg) :
        callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{arg}, state_f{ThreadState::CREATED},
        pthread_f{0}, pthreadAttr_f{}, identity_f{name}, runnableInitialized_f{false} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            Callable<CallableArg_TP> castCallable = reinterpret_cast<Callable<CallableArg_TP>>(erasedCallable);
            CallableArg_TP* castArg = reinterpret_cast<CallableArg_TP*>(erasedArg);
            castCallable(castArg);
        };
    }

    Thread(ThreadName name, const CallableNoArg& callable) :
        callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{nullptr}, state_f{ThreadState::CREATED},
        pthread_f{0}, pthreadAttr_f{}, identity_f{name}, runnableInitialized_f{false} {
        this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
            CallableNoArg castCallable = reinterpret_cast<CallableNoArg>(erasedCallable);
            castCallable();
        };
    }

public:  // copy-control
    Thread(const Thread& rhs) = delete;

    Thread(Thread&& rhs) noexcept = default;

    Thread&
    operator=(const Thread& rhs) = delete;

    Thread&
    operator=(Thread&& rhs) noexcept = default;

    ~Thread() {
        this->kill();
    }

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
    const ThreadIdentity&
    identity() const& {
        return this->identity_f;
    }

public:  // member functions
    void
    start() & {
        if (this->state() == ThreadState::RUNNING) {
            return;
        }

        ::pthread_attr_init(&this->pthreadAttr_f);

        int result = ::pthread_create(&this->pthread_f, &this->pthreadAttr_f, &Thread::runnable, this);
        ::pthread_attr_destroy(&this->pthreadAttr_f);
        if (result != 0) {
            return;
        }

        // Thread name must be 16 characters including null terminator according to Linux docs,
        //  see https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
        char nameBuffer[16];

        // strncpy pads with 0 if src is shorter than expected
        std::strncpy(nameBuffer, this->identity_f.name.data(), 15);
        nameBuffer[15] = '\0';  // Enforce null termination
        ::pthread_setname_np(this->pthread_f, nameBuffer);

        while (!this->runnableInitialized_f.load(std::memory_order_acquire)) {
            // spin until runnable has initialized such that identity and any other state stuff has been initialized
        }
    }

    void
    kill() & {
        if (this->state() != ThreadState::RUNNING) {
            return;
        }

        ::pthread_cancel(this->pthread_f);

        this->state_f.store(ThreadState::KILLED, std::memory_order_release);
        this->pthread_f = 0;
    }

private:  // member functions
    static void*
    runnable(void* arg) {
        Thread* thread = reinterpret_cast<Thread*>(arg);
        thread->state_f.store(ThreadState::RUNNING, std::memory_order_release);
        thread->identity_f.id = Thread::calling_thread_id();

        ::pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        ::pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

        thread->runnableInitialized_f.store(true, std::memory_order_release);
        thread->invoker_f(thread->callable_f, thread->callableArg_f);

        thread->state_f.store(ThreadState::FINISHED, std::memory_order_release);
        return nullptr;
    }

public:  // static functions
    [[nodiscard]]
    static ThreadId
    calling_thread_id() {
        return ::gettid();
    }

    [[nodiscard]]
    static ThreadId
    main_thread_id() {
        return ::getpid();
    }
};

}  // namespace pican
