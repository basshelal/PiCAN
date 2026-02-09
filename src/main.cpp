#include <csignal>

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "noheap/NoHeap.hpp"
#include "pican/Array.hpp"
#include "pican/ThreadManager.hpp"
#include "pican/log/LoggerThread.hpp"
#include "pican/log/Sink.hpp"
#include "pican/mem/Manager.hpp"
#include "pican/time/DateTime.hpp"
#include "stacktrace/StackTrace.hpp"

void
initialize(int argc, char** argv) {
    // stacktrace
    stacktrace::initialize(argv);
    ::signal(SIGSEGV, ::stacktrace::signal_handler);
    ::signal(SIGILL, ::stacktrace::signal_handler);
    ::signal(SIGABRT, ::stacktrace::signal_handler);

    // disable heap
    noheap::seal_heap();
    assert(noheap::heap_is_sealed());

    pican::mem::Manager::initialize(pican::mem::Manager::DEFAULT_SIZE);

    pican::ThreadManager::initialize();
    pican::ThreadManager::start_all_threads();

    printf("From initialize: %d", pican::Thread::calling_thread());

    pican::mem::Manager::seal();
}

int
main(int argc, char** argv) {
    initialize(argc, argv);

    struct timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    const auto oldSecs = ts.tv_sec;

    pican::Thread::Callable<struct timespec> runnable = [](struct timespec* arg) -> void {
        printf("From runnable: %d", pican::Thread::calling_thread());
        pican::log::LoggerThread::register_thread(pican::Thread::calling_thread());
        while (true) {
            int min = 1'000'000 / 2;
            int max = 1'000'000;
            int range = max - min + 1;
            int num = rand() % range + min;
            ::usleep(num);
            pican::log::LoggerThread::log(pican::log::Level::ERROR, "{}:{}", arg->tv_sec, arg->tv_nsec);
        }
    };

    pican::log::LoggerThread::start_thread();

    pican::log::LoggerThread::log(pican::log::Level::ERROR, "{}", 420);

    pican::Thread thread{"Example Thread", runnable, &ts};

    thread.start();

    while (ts.tv_sec <= (oldSecs + 10)) {
        ::clock_gettime(CLOCK_REALTIME, &ts);
        ::usleep(1'000);
    }

    thread.stop();
}
