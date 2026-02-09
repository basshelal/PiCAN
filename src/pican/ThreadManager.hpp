#pragma once

#include <array>

#include "log/LoggerThread.hpp"
#include "pican/Thread.hpp"

namespace pican {
constexpr Count MAX_THREADS_COUNT = 8;

class ThreadManager {
private:  // types
    using This = ThreadManager;

    struct ThreadIdNamePair {
        ThreadId id;
        ThreadName name;
    };

private:  // static fields
    static ThreadManager* instance_sf;

private:  // fields
    bool started_f;
    std::array<ThreadIdNamePair, MAX_THREADS_COUNT> threads_f;
    Count threadsCount_f;

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
    start_all_threads();

    static ThreadName
    get_thread_name_from_id(ThreadId id);

    static ThreadId
    get_thread_id_from_name(ThreadName name);

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
