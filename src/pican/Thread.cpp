#include "Thread.hpp"

namespace pican {
Thread::~Thread() {
    this->stop();
    if (this->pthread_f != 0) {
        ::pthread_join(this->pthread_f, nullptr);
    }
}

void*
Thread::pthread_runnable(void* arg) {
    Thread* thread = reinterpret_cast<Thread*>(arg);

    ::pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    ::pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    thread->threadId_f = ::gettid();

    thread->invoker_f(thread->callable_f, thread->callableArg_f);

    thread->state_f.store(ThreadState::STOPPED, std::memory_order_release);
    return nullptr;
}

void
Thread::start() {
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
}

void
Thread::stop() {
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
