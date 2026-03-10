#include "pican/Application.hpp"

#include <magic_enum/magic_enum.hpp>
#include <time.h>

#include "pican/File.hpp"
#include "pican/Log.hpp"
#include "pican/can/CanThread.hpp"
#include "pican/mem/Manager.hpp"

using TimeSpec = struct ::timespec;

namespace pican {
Application* Application::instance_sf = nullptr;

Application::Application() :
    state_f{State::INITIALIZED}, stdout_f{"/dev/stdout"}, stderr_f{"/dev/stderr"},
    threads_f{mem::Manager::get_array<IApplicationThread*>(pican::config::THREADS_COUNT)} {
}

/* static */
void
Application::initialize() {
    if (This::instance_sf != nullptr) {
        pican::panic("Application already initialized!");
    }

    mem::Block block = mem::Manager::get_block_for<Application>();
    Application* ptr = block.address_to_ptr<Application>();

    This::instance_sf = new (ptr) Application{};

    Application& instance = This::get_instance();

    // create LoggerThread
    mem::Block loggerThreadBlock = mem::Manager::get_block_for<log::LoggerThread>();

    pican::Result<log::LoggerThread*, log::LoggerThread::Error> loggerThreadResult =
        log::LoggerThread::create(loggerThreadBlock, log::Level::VERBOSE, "log", 8, 1'024);

    if (loggerThreadResult.is_failure()) {
        [[maybe_unused]]
        log::LoggerThread::Error error = loggerThreadResult.failure_value_or_panic();
        pican::panic("Failed!");
    }
    log::LoggerThread* loggerThreadPtr = loggerThreadResult.success_value_or_panic();
    CONTRACTS_ASSERT(loggerThreadPtr == loggerThreadBlock.address_to_ptr<log::LoggerThread>());
    instance.loggerThread_f = loggerThreadPtr;

    pican::log_function_g = [loggerThreadPtr](const log::Entry& entry) -> void {
        loggerThreadPtr->log_entry(entry);
    };

    instance.threads_f.add_copy(instance.loggerThread_f);

    // initialize LoggerThread
    pican::Result<log::Sink, log::Sink::Error> stdoutSinkResult =
        log::Sink::create("stdout", log::Level::VERBOSE, "/dev/stdout");
    [[maybe_unused]]
    auto res1 = instance.loggerThread_f->register_sink(std::move(stdoutSinkResult.success_value_or_panic()));

    pican::log_info("stdout sink added");

    pican::Result<log::Sink, log::Sink::Error> stderrSinkResult =
        log::Sink::create("stderr", log::Level::ERROR, "/dev/stderr");
    [[maybe_unused]]
    auto res2 = instance.loggerThread_f->register_sink(std::move(stderrSinkResult.success_value_or_panic()));

    pican::log_info("stderr sink added");

    const ThreadIdentity mainThreadIdentity = ThreadIdentity{"main", Thread::calling_thread_id()};
    [[maybe_unused]]
    auto res3 = instance.loggerThread_f->register_thread(mainThreadIdentity);

    // create CanThread
    mem::Block canThreadBlock = mem::Manager::get_block_for<can::CanThread>();

    Array<can::Event> uiRingBufferArray = mem::Manager::get_array<can::Event>(8'192);
    Array<can::Event> netRingBufferArray = mem::Manager::get_array<can::Event>(8'192);

    pican::Result<can::CanThread*, can::CanThread::Error> canThreadResult = can::CanThread::create(
        canThreadBlock, config::CAN_INTERFACE_NAME, "can", config::CAN_LINUX_BUFFER_SIZE,
        config::CAN_THREAD_TIMEOUT_SECONDS, uiRingBufferArray, netRingBufferArray
    );

    if (canThreadResult.is_failure()) {
        [[maybe_unused]]
        can::CanThread::Error error = canThreadResult.failure_value_or_panic();
        pican::panic("Failed!");
    }
    can::CanThread* canThreadPtr = canThreadResult.success_value_or_panic();
    CONTRACTS_ASSERT(canThreadPtr == canThreadBlock.address_to_ptr<can::CanThread>());
    instance.canThread_f = canThreadPtr;

    instance.threads_f.add_copy(instance.canThread_f);


    // create info thread

}

/* static */
void
Application::start() {
    Application& instance = This::get_instance();
    if (instance.state_f == State::RUNNING || instance.state_f == State::SLEEPING) {
        return;
    }

    instance.loggerThread_f->start();
    instance.canThread_f->start();

    instance.state_f = State::RUNNING;

    [[maybe_unused]]
    auto res0 = instance.loggerThread_f->register_thread(instance.loggerThread_f->thread_identity());
    [[maybe_unused]]
    auto res1 = instance.loggerThread_f->register_thread(instance.canThread_f->thread_identity());
}

/* static */
void
Application::stop() {
    Application& instance = This::get_instance();
    instance.loggerThread_f->stop();
    instance.canThread_f->stop();

    instance.state_f = State::STOPPED;
}

/* static */
void
Application::loop() {
    Application& instance = Application::get_instance();
    std::array<ThreadCounterValue, config::THREADS_COUNT> counters;
    for (ThreadCounterValue& counter : counters) {
        counter = 0;
    }
    while (instance.state_f != State::STOPPED) {
        instance.state_f = State::RUNNING;
        pican::log_info("Looping!");
        for (Count i = 0; i < instance.threads_f.size(); ++i) {
            const IApplicationThread* thread = instance.threads_f[i];
            const ThreadCounterValue oldValue = counters[i];
            const ThreadCounterValue newValue = thread->thread_counter_value();
            if (oldValue == newValue) {
                pican::log_warn(
                    "Counter values unchanged for thread: {}, values: {}", thread->thread_identity().name, oldValue
                );
            }
        }
        // TODO @basshelal Wed 18-Feb-2026 : Better implementation of checking if a thread is hung and do something
        //  a thread is hanging, log it, print the stacktrace and exit gracefully by means of cleanup, maybe by just
        //  sending a signal? We need to dump as much info as possible though

        instance.state_f = State::SLEEPING;
        pican::log_info("Sleeping!");
        ::usleep(config::MAIN_LOOP_SLEEP_MILLISECONDS * 1'000);
    }
}

/* static */
ThreadIdentity
Application::calling_thread() {
    Application& instance = This::get_instance();
    const ThreadId currentThreadId = Thread::calling_thread_id();
    for (const IApplicationThread* thread : instance.threads_f) {
        const ThreadIdentity& identity = thread->thread_identity();
        if (identity.id == currentThreadId) {
            return identity;
        }
    }
    return ThreadIdentity{"Main", Thread::main_thread_id()};
}

/* static */
bool
Application::is_initialized() {
    return This::instance_sf != nullptr;
}

/* static */
void
Application::ensure_initialized() {
    if (This::instance_sf == nullptr) {
        pican::panic("Application not initialized!");
    }
}

/* static */
Application&
Application::get_instance() {
    This::ensure_initialized();
    return *This::instance_sf;
}

}  // namespace pican
