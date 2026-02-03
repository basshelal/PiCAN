#include <csignal>
#include <thread>

#include <magic_enum/magic_enum.hpp>
#include <pican/LoggerThread.hpp>
#include <unistd.h>

#include "noheap/NoHeap.hpp"
#include "pican/Array.hpp"
#include "pican/Logger.hpp"
#include "pican/ThreadManager.hpp"
#include "pican/memory/Manager.hpp"
#include "stacktrace/StackTrace.hpp"

// static pican::EventNotifier eventNotifier{};
//
// static pican::RingBuffer<pican::can::Frame> ringBuffer{
//     65'536,
//     pican::RingBufferOverflowBehavior::DEFAULT,
//     [](const pican::RingBuffer<pican::can::Frame>&, const pican::can::Frame& frame) -> void {
//         fmt::println(fmt::runtime("Overflow! {}"), to_string(frame));
//     },
//     [](const pican::RingBuffer<pican::can::Frame>&) -> void {
//         fmt::println(stderr, "Underflow!");
//     },
// };

// static void
// consumerThreadRunnable() {
// while (eventNotifier.is_active()) {
//     eventNotifier.wait_blocking();
//     pican::can::Frame* maybeFrame = ringBuffer.pop();
//     if (maybeFrame != nullptr) {
//         fmt::println(to_string(*maybeFrame));
//     }
// }
// }

// static void
// producerCallback(const pican::can::Frame& frame) {
// ringBuffer.push_copy(frame);
// eventNotifier.notify();
// }

void initialize(int argc, char**argv) {
    // stacktrace
    stacktrace::initialize(argv);
    ::signal(SIGSEGV, ::stacktrace::signal_handler);
    ::signal(SIGILL, ::stacktrace::signal_handler);
    ::signal(SIGABRT, ::stacktrace::signal_handler);

    // disable heap
    noheap::seal_heap();
    assert(noheap::heap_is_sealed());

    pican::memory::Manager::initialize(pican::memory::Manager::DEFAULT_SIZE);

    pican::ThreadManager::initialize_all();

    pican::Logger::initialize(1'024, 8, pican::Level::VERBOSE);

    pican::Logger::register_log_writer(pican::LogWriter{"stdout", pican::Level::VERBOSE, STDOUT_FILENO});

    pican::ThreadManager::start_all();

    pican::LoggerThread::initialize();
    pican::LoggerThread::start();

    pican::memory::Manager::get().seal();
}

int
main(int argc, char** argv) {

    initialize(argc, argv);

    struct timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    const auto oldSecs = ts.tv_sec;

    pican::Thread::Callable<struct timespec> runnable = [](struct timespec* arg) -> void {
        while (true) {
            // fmt::println("{}", arg->tv_sec);
            int min = 1'000'000 / 2;
            int max = 2'000'000;
            int range = max - min + 1;
            int num = rand() % range + min;
            ::usleep(num);
            pican::Logger::log(pican::Level::ERROR, "{}:{}", arg->tv_sec, arg->tv_nsec);
        }
    };

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

    //
    // std::thread canFramesConsumer{consumerThreadRunnable};
    //
    // pican::can::ReaderThread readerThread{"vcan0", producerCallback};
    //
    // readerThread.start();
    //
    // sleep(10);
    //
    // eventNotifier.close();
    //
    // sleep(10);
}
