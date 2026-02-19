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
    state_f{State::INITIALIZED}, stdout_f{"/dev/stdout", File::Mode::WRITE, mem::Manager::get_block(8'192)},
    stderr_f{"/dev/stderr", File::Mode::WRITE, mem::Manager::get_block(8'192)},
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
        log::LoggerThread::create(loggerThreadBlock, log::Level::VERBOSE, "Logger", 8, 1'024);

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
    instance.stdout_f.open();
    const log::Sink stdoutSink{"stdout", log::Level::VERBOSE, instance.stdout_f};
    [[maybe_unused]]
    auto res1 = instance.loggerThread_f->register_sink(stdoutSink);

    pican::log_info("stdout sink added");

    instance.stderr_f.open();
    const log::Sink stderrSink{"stderr", log::Level::ERROR, instance.stderr_f};
    [[maybe_unused]]
    auto res2 = instance.loggerThread_f->register_sink(stderrSink);

    pican::log_info("stderr sink added");

    const ThreadIdentity mainThreadIdentity = ThreadIdentity{Thread::main_thread_id(), "Main"};
    [[maybe_unused]]
    auto res3 = instance.loggerThread_f->register_thread(mainThreadIdentity);

    // create CanThread
    mem::Block canThreadBlock = mem::Manager::get_block_for<can::CanThread>();
    can::CanThread* canThreadPtr = canThreadBlock.address_to_ptr<can::CanThread>();

    Array<can::Event> uiRingBufferArray = mem::Manager::get_array<can::Event>(8'192);
    Array<can::Event> netRingBufferArray = mem::Manager::get_array<can::Event>(8'192);
    instance.canThread_f = new (canThreadPtr) can::CanThread{"vcan0", uiRingBufferArray, netRingBufferArray};

    instance.threads_f.add_copy(instance.canThread_f);
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
    return ThreadIdentity{Thread::main_thread_id(), "Main"};
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
