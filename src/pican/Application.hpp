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

public:  // friends
    template<typename... Args_TP>
    inline friend void
    log_function(pican::log::Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args);
};

template<typename... Args_TP>
inline void
log_function(pican::log::Level level, fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    Application& application = Application::get_instance();
    application.loggerThread_f->log(level, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_error(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    pican::log_function(pican::log::Level::ERROR, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_warn(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    pican::log_function(pican::log::Level::WARN, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_info(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    pican::log_function(pican::log::Level::INFO, format, std::forward<Args_TP>(args)...);
}

template<typename... Args_TP>
inline void
log_verbose(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    pican::log_function(pican::log::Level::VERBOSE, format, std::forward<Args_TP>(args)...);
}
}  // namespace pican
