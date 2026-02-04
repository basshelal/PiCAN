#pragma once

#include <array>

#include "log/LoggerThread.hpp"
#include "pican/Thread.hpp"

namespace pican {
class ThreadManager {
private:  // types
    using This = ThreadManager;

private:  // static fields
    static ThreadManager* instance_sf;

private:  // fields
    log::LoggerThread *logger_f;

private:  // constructor
    ThreadManager() = default;

public:  // copy-control
    ThreadManager(const ThreadManager& rhs) = delete;

    ThreadManager(ThreadManager&& rhs) noexcept = delete;

    ThreadManager&
    operator=(const ThreadManager& rhs) = delete;

    ThreadManager&
    operator=(ThreadManager&& rhs) noexcept = delete;

    ~ThreadManager() = default;

public:  // static functions
    static void
    initialize();

    static void
    initialize_all();

    static void
    start_all();

    [[nodiscard]]
    static ThreadId
    calling_thread();

public:  // getters
    [[nodiscard]]
    static bool
    is_initialized();

private:  // helper functions
    static void
    ensure_initialized();
};

// TODO @basshelal Tue 03-Feb-2026 : Current (calling) thread name/id/object??

}  // namespace pican
