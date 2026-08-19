module;

#include <array>
#include <cstdint>
#include <ctime>

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "pican/contracts.hpp"

export module pican:Application;

import pican.core;
import pican.info;
import pican.log;
import pican.ds;
import pican.fs;
import pican.can;
import pican.mem;

export namespace pican {

class Application {
public:  // types
    enum class Error : std::uint8_t {
    };

    using Result = pican::SimpleResult<Error>;

    enum class State : std::uint8_t {
        INITIALIZED,
        RUNNING,
        SLEEPING,
        STOPPED,
    };

private:  // types
    using This = Application;

private:  // static fields
    static Application* instance_sf;

private:  // fields
    State state_f;
    fs::File stdout_f;
    fs::File stderr_f;
    ds::ArrayList<IApplicationThread*> threads_f;
    log::LoggerThread* loggerThread_f;
    can::CanThread* canThread_f;
    info::InfoThread* infoThread_f;

private:  // constructor
    Application() :
        state_f{State::INITIALIZED}, stdout_f{"/dev/stdout"}, stderr_f{"/dev/stderr"},
        threads_f{mem::Manager::get_array<IApplicationThread*>(pican::THREADS_COUNT, nullptr)} {
    }

public:  // lifetime
    Application(const Application& rhs) = delete;

    Application(Application&& rhs) noexcept = delete;

    Application&
    operator=(const Application& rhs) & = delete;

    Application&
    operator=(Application&& rhs) & noexcept = delete;

    ~Application() = default;

public:  // static functions
    static void
    initialize() {
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

        ds::Array<can::Event> uiRingBufferArray = mem::Manager::get_array<can::Event>(8'192, can::Event{});
        ds::Array<can::Event> netRingBufferArray = mem::Manager::get_array<can::Event>(8'192, can::Event{});

        pican::Result<can::CanThread*, can::CanThread::Error> canThreadResult = can::CanThread::create(
            canThreadBlock, pican::CAN_INTERFACE_NAME, "can", pican::CAN_LINUX_BUFFER_SIZE,
            pican::CAN_THREAD_TIMEOUT_SECONDS, uiRingBufferArray, netRingBufferArray
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
        mem::Block infoThreadBlock = mem::Manager::get_block_for<info::InfoThread>();

        mem::Block fileBuffer = mem::Manager::get_block(8'192);

        pican::Result<info::InfoThread*, info::InfoThread::Error> infoThreadResult =
            info::InfoThread::create(infoThreadBlock, fileBuffer, 500);

        if (infoThreadResult.is_failure()) {
            [[maybe_unused]]
            info::InfoThread::Error error = infoThreadResult.failure_value_or_panic();
            pican::panic("Failed!");
        }
        info::InfoThread* infoThreadPtr = infoThreadResult.success_value_or_panic();
        CONTRACTS_ASSERT(infoThreadPtr == infoThreadBlock.address_to_ptr<info::InfoThread>());
        instance.infoThread_f = infoThreadPtr;

        instance.threads_f.add_copy(instance.infoThread_f);
    }

    static void
    start() {
        Application& instance = This::get_instance();
        if (instance.state_f == State::RUNNING || instance.state_f == State::SLEEPING) {
            return;
        }

        instance.loggerThread_f->start();
        instance.canThread_f->start();
        instance.infoThread_f->start();

        instance.state_f = State::RUNNING;

        [[maybe_unused]]
        auto res0 = instance.loggerThread_f->register_thread(instance.loggerThread_f->thread_identity());
        [[maybe_unused]]
        auto res1 = instance.loggerThread_f->register_thread(instance.canThread_f->thread_identity());
        [[maybe_unused]]
        auto res2 = instance.loggerThread_f->register_thread(instance.infoThread_f->thread_identity());
    }

    static void
    stop() {
        Application& instance = This::get_instance();
        instance.loggerThread_f->stop();
        instance.canThread_f->stop();
        instance.infoThread_f->stop();

        instance.state_f = State::STOPPED;
    }

    static void
    loop() {
        Application& instance = Application::get_instance();
        std::array<ThreadCounterValue, pican::THREADS_COUNT> counters;
        for (ThreadCounterValue& counter : counters) {
            counter = 0;
        }
        while (instance.state_f != State::STOPPED) {
            instance.state_f = State::RUNNING;
            pican::log_info("Looping!");
            for (Count i = 0; i < instance.threads_f.size(); ++i) {
                const IApplicationThread* thread = instance.threads_f[i];
                const ThreadCounterValue oldValue = counters[i];
                const ThreadCounterValue newValue = thread->counter_value();
                pican::log_info(
                    "Thread: {} :: {} -> {}", thread->thread_identity().name, oldValue, thread->counter_value()
                );
                if (oldValue == newValue) {
                    pican::log_warn(
                        "Counter values unchanged for thread: {}, values: {}", thread->thread_identity().name, oldValue
                    );
                }
                counters[i] = thread->counter_value();
            }
            // TODO @basshelal Wed 18-Feb-2026 : Better implementation of checking if a thread is hung and do something
            //  a thread is hanging, log it, print the stacktrace and exit gracefully by means of cleanup, maybe by just
            //  sending a signal? We need to dump as much info as possible though

            instance.state_f = State::SLEEPING;
            pican::log_info("Sleeping!");
            ::usleep(pican::MAIN_LOOP_SLEEP_MILLISECONDS * 1'000);
        }
    }

    [[nodiscard]]
    static ThreadIdentity
    calling_thread() {
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

public:  // getters
    [[nodiscard]]
    static bool
    is_initialized() {
        return This::instance_sf != nullptr;
    }

private:  // helper functions
    static void
    ensure_initialized() {
        if (This::instance_sf == nullptr) {
            pican::panic("Application not initialized!");
        }
    }

    [[nodiscard]]
    static Application&
    get_instance() {
        This::ensure_initialized();
        return *This::instance_sf;
    }
};

Application* Application::instance_sf = nullptr;
}  // namespace pican
