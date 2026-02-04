#include <csignal>

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "noheap/NoHeap.hpp"
#include "pican/Array.hpp"
#include "pican/ThreadManager.hpp"
#include "pican/log/Logger.hpp"
#include "pican/log/LoggerThread.hpp"
#include "pican/mem/Manager.hpp"
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

    pican::log::LoggerThread::initialize(pican::log::Level::VERBOSE,8,8);

    pican::log::LoggerThread::register_logger(pican::log::Logger{"stdout", pican::log::Level::VERBOSE, STDOUT_FILENO});
    pican::log::LoggerThread::register_thread(pican::Thread::calling_thread(), 256);

    pican::log::LoggerThread::start_thread();

    // pican::ThreadManager::initialize();

    // pican::ThreadManager::initialize_all();

    // pican::ThreadManager::start_all();

    // pican::mem::Manager::seal();
}

int
main(int argc, char** argv) {
    initialize(argc, argv);

    struct timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    const auto oldSecs = ts.tv_sec;

    pican::Thread::Callable<struct timespec> runnable = [](struct timespec* arg) -> void {
        pican::log::LoggerThread::register_thread(pican::Thread::calling_thread(), 256);
        while (true) {
            int min = 1'000'000 / 2;
            int max = 2'000'000;
            int range = max - min + 1;
            int num = rand() % range + min;
            ::usleep(num);
            pican::log::LoggerThread::log(pican::log::Level::ERROR, "{}:{}", arg->tv_sec, arg->tv_nsec);
        }
    };

    pican::log::LoggerThread::log(pican::log::Level::ERROR, "{}", 420);

    pican::Thread thread{"Example Thread", runnable, &ts};
    fmt::println("{}", magic_enum::enum_name(thread.state()));

    thread.start();

    fmt::println("{}", magic_enum::enum_name(thread.state()));

    while (ts.tv_sec <= (oldSecs + 10)) {
        ::clock_gettime(CLOCK_REALTIME, &ts);
        ::usleep(1'000);
    }

    thread.stop();

    fmt::println("{}", magic_enum::enum_name(thread.state()));
}
