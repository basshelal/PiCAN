#pragma once

#include <array>
#include <cstdint>

#include "pican/Config.hpp"
#include "pican/IApplicationThread.hpp"
#include "pican/Result.hpp"
#include "pican/Thread.hpp"
#include "pican/can/CanThread.hpp"
#include "pican/log/LoggerThread.hpp"

namespace pican {

enum class Threads : std::uint8_t {
    MAIN,
    LOGGER,
    CAN,
};

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
    File stdout_f;
    File stderr_f;
    ArrayList<IApplicationThread*> threads_f;
    log::LoggerThread* loggerThread_f;
    can::CanThread* canThread_f;

private:  // constructor
    Application();

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
    initialize();

    static void
    start();

    static void
    stop();

    static void
    loop();

    [[nodiscard]]
    static ThreadIdentity
    calling_thread();

public:  // getters
    [[nodiscard]]
    static bool
    is_initialized();

private:  // helper functions
    static void
    ensure_initialized();

    [[nodiscard]]
    static Application&
    get_instance();
};
}  // namespace pican
