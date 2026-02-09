#include "pican/ThreadManager.hpp"

#include <pthread.h>

#include "pican/mem/Manager.hpp"

namespace pican {
ThreadManager* ThreadManager::instance_sf = nullptr;

/* static */
void
ThreadManager::initialize() {
    if (This::instance_sf != nullptr) {
        pican::panic("ThreadManager already initialized!");
    }
    mem::Block block = mem::Manager::get_block(sizeof(ThreadManager));

    This::instance_sf = new (block.address_to_ptr<ThreadManager>()) ThreadManager{};

    This::instance_sf->threads_f[0] = {Thread::calling_thread(), "Main"};
    This::instance_sf->threadsCount_f = 1;

    // initialize LoggerThread
    log::LoggerThread::initialize(log::Level::VERBOSE, 8, 8, 1'024);

    File stdoutFile{"/dev/stdout", File::Mode::WRITE, mem::Manager::get_block(8'192)};
    stdoutFile.open();
    const log::Sink stdoutLogger{"stdout", log::Level::VERBOSE, stdoutFile};
    log::LoggerThread::register_logger(stdoutLogger);

    log::LoggerThread::register_thread(Thread::calling_thread());
    This::instance_sf->threads_f[This::instance_sf->threadsCount_f++] = {
        Thread::calling_thread(), "Main"
    };
}

/* static */
void
ThreadManager::start_all_threads() {
    This::ensure_initialized();
    if (This::instance_sf->started_f) {
        return;
    }
    log::LoggerThread::start_thread();
    This::instance_sf->threads_f[This::instance_sf->threadsCount_f++] = {
        log::LoggerThread::instance_sf->thread_f.threadId_f, "Logger"
    };

    This::instance_sf->started_f = true;
}

ThreadName
ThreadManager::get_thread_name_from_id(ThreadId id) {
    for (Count i = 0; i < This::instance_sf->threadsCount_f; ++i) {
        const ThreadIdNamePair& pair = This::instance_sf->threads_f[i];
        if (pair.id == id) {
            return pair.name;
        }
    }
    return "Unknown";
}

ThreadId
ThreadManager::get_thread_id_from_name(ThreadName name) {
    for (Count i = 0; i < This::instance_sf->threadsCount_f; ++i) {
        const ThreadIdNamePair& pair = This::instance_sf->threads_f[i];
        if (pair.name == name) {
            return pair.id;
        }
    }
    return 0;
}

/* static */
bool
ThreadManager::is_initialized() {
    return This::instance_sf != nullptr;
}

/* static */
void
ThreadManager::ensure_initialized() {
    if (This::instance_sf == nullptr) {
        pican::panic("ThreadManager not initialized!");
    }
}

}  // namespace pican
