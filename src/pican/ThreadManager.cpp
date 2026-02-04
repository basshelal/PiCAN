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
    This::instance_sf->logger_f = log::LoggerThread::instance_sf;
}

/* static */
void
ThreadManager::initialize_all() {
}

/* static */
void
ThreadManager::start_all() {
}

/* static */
ThreadId
ThreadManager::calling_thread() {
    return ::pthread_self();
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
