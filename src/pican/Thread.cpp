#include "Thread.hpp"

#include <cstring>

namespace pican {

Thread::Thread(ThreadName name, const CallableNoArg& callable) :
    callable_f{reinterpret_cast<ErasedCallable>(callable)}, callableArg_f{nullptr}, state_f{ThreadState::CREATED},
    pthread_f{0}, pthreadAttr_f{}, identity_f{name} {
    this->invoker_f = [](ErasedCallable erasedCallable, void* erasedArg) -> void {
        CallableNoArg castCallable = reinterpret_cast<CallableNoArg>(erasedCallable);
        castCallable();
    };
}

Thread::~Thread() {
    this->stop();
    if (this->pthread_f != 0) {
        ::pthread_join(this->pthread_f, nullptr);
    }
}

ThreadState
Thread::state() const& {
    return this->state_f.load(std::memory_order_acquire);
}

bool
Thread::is_running() const& {
    return this->state() == ThreadState::RUNNING;
}

const ThreadIdentity&
Thread::thread_identity() const& {
    return this->identity_f;
}

void*
Thread::pthread_runnable(void* arg) {
    Thread* thread = reinterpret_cast<Thread*>(arg);
    thread->identity_f.id = Thread::calling_thread_id();

    ::pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    ::pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    thread->invoker_f(thread->callable_f, thread->callableArg_f);

    thread->state_f.store(ThreadState::STOPPED, std::memory_order_release);
    return nullptr;
}

void
Thread::start() & {
    const ThreadState oldState = this->state();
    if (oldState == ThreadState::RUNNING) {
        return;
    }
    this->state_f.store(ThreadState::RUNNING, std::memory_order_release);

    ::pthread_attr_init(&this->pthreadAttr_f);

    int result = ::pthread_create(&this->pthread_f, &this->pthreadAttr_f, &Thread::pthread_runnable, this);
    ::pthread_attr_destroy(&this->pthreadAttr_f);
    if (result != 0) {
        this->state_f.store(oldState, std::memory_order_release);
        return;
    }

    // Thread name must be 16 characters including null terminator according to Linux docs,
    //  see https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
    char nameBuffer[16];

    // strncpy pads with 0 if src is shorter than expected
    std::strncpy(nameBuffer, this->identity_f.name.data(), 15);
    nameBuffer[15] = '\0';  // Enforce null termination
    ::pthread_setname_np(this->pthread_f, nameBuffer);
}

// TODO @basshelal Fri 20-Feb-2026 : This could possibly be a bad idea!
//  instead we should ALWAYS design our IApplicationThreads to use a isRunning flag which will
//  be checked every loop iteration, since most threads will use an EventFD anyway, the stop() function
//  in the IApplicationThread can just toggle the flag and notify the eventFD instead of just force killing
//  killing the pthread, this allows for custom cleanup too by every IApplicationThread
void
Thread::stop() & {
    if (this->state() != ThreadState::RUNNING) {
        return;
    }

    ::pthread_cancel(this->pthread_f);

    // Wait for it to actually die
    // This is crucial to prevent zombie threads and reclaim stack memory.
    ::pthread_join(this->pthread_f, nullptr);

    this->state_f.store(ThreadState::STOPPED, std::memory_order_release);
    this->pthread_f = 0;
}

/* static */
ThreadId
Thread::calling_thread_id() {
    return ::gettid();
}

/* static */
ThreadId
Thread::main_thread_id() {
    return ::getpid();
}

}  // namespace pican
