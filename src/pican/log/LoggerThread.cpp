#include "LoggerThread.hpp"

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "LoggerThread.hpp"

namespace pican::log {
LoggerThread* LoggerThread::instance_sf = nullptr;

LoggerThread::LoggerThread(Level level, Count maxBuffersCount, Count maxLoggersCount) :
    level_f{level}, buffers_f{mem::Manager::get_array<Buffer>(maxBuffersCount)}, buffersCount_f{0},
    loggers_f{mem::Manager::get_array<Logger>(maxLoggersCount)}, loggersCount_f{0}, eventfd_f{EventFD::Mode::NOTIFY},
    thread_f{"LoggerThread", &LoggerThread::runnable, this} {
}

/* static */
void
LoggerThread::initialize(Level level, Count maxBuffersCount, Count maxLoggersCount) {
    if (This::instance_sf != nullptr) {
        pican::panic("LoggerThread already initialized!");
    }

    mem::Block block = mem::Manager::get_block(sizeof(LoggerThread));

    This::instance_sf =
        new (block.address_to_ptr<LoggerThread>()) LoggerThread{level, maxBuffersCount, maxLoggersCount};

    This::ensure_initialized();
}

/* static */
void
LoggerThread::register_logger(const Logger& logger) {
    This::ensure_initialized();
    LoggerThread& instance = *This::instance_sf;
    if (instance.loggersCount_f >= instance.loggers_f.items_count()) {
        pican::panic("Loggers at capacity");
    }
    instance.loggers_f.set_at(instance.loggersCount_f, logger);
    ++instance.loggersCount_f;
}

/* static */
void
LoggerThread::register_thread(ThreadId threadId, Count bufferEntryCount) {
    This::ensure_initialized();
    LoggerThread& instance = *This::instance_sf;
    Buffer* found = instance.get_buffer_of_thread(threadId);
    if (found != nullptr) {
        pican::panic("The thread is already registered!");
    }
    if (instance.buffersCount_f >= instance.buffers_f.items_count()) {
        pican::panic("Buffers at capacity!");
    }

    Buffer* uninitializedBuffer = instance.buffers_f.get_at_ptr(instance.buffersCount_f);
    uninitializedBuffer = new (uninitializedBuffer) Buffer{threadId, bufferEntryCount};

    ++instance.buffersCount_f;
}

/* static */
void
LoggerThread::start_thread() {
    This::ensure_initialized();
    This::instance_sf->thread_f.start();
}

/* static */
void
LoggerThread::runnable(LoggerThread* self) {
    This::ensure_initialized();
    LoggerThread& instance = *This::instance_sf;
    while (true) {
        instance.eventfd_f.wait_blocking();

        // TODO @basshelal Wed 04-Feb-2026 : This is a pretty good candidate for epoll,
        //  many threads need to "notify" this thread that it needs to wake up and do stuff

        for (Index bufferIndex = 0; bufferIndex < instance.buffersCount_f; ++bufferIndex) {
            Buffer* buffer = instance.buffers_f.get_at_ptr(bufferIndex);
            while (!buffer->entries().is_empty()) {
                for (Index loggerIndex = 0; loggerIndex < instance.loggersCount_f; ++loggerIndex) {
                    Logger* logger = instance.loggers_f.get_at_ptr(loggerIndex);
                    std::optional<Entry> entryOptional = buffer->entries().pop_move();
                    if (!entryOptional.has_value()) {
                        continue;
                    }
                    const Entry& entry = entryOptional.value();


                    if (logger->level() < entry.level) {
                        continue;
                    }
                    char buffer[Entry::ACTUAL_MESSAGE_ENTRY_SIZE];
                    ::snprintf(buffer, sizeof(buffer), "[%zu] ", entry.timestamp);
                    ::write(logger->file_descriptor(), buffer, strlen(buffer));

                    ::snprintf(buffer, sizeof(buffer), "%s: ", magic_enum::enum_name(entry.level).data());
                    ::write(logger->file_descriptor(), buffer, strlen(buffer));

                    ::snprintf(buffer, sizeof(buffer), "%s\n", entry.message.data());
                    ::write(logger->file_descriptor(), buffer, strlen(buffer));
                }
            }
        }
    }
}

Buffer*
LoggerThread::get_buffer_of_thread(ThreadId threadId) const& {
    for (Index i = 0; i < this->buffersCount_f; ++i) {
        Buffer* buffer = this->buffers_f.get_at_ptr(i);
        if (buffer->thread_id() == threadId) {
            return buffer;
        }
    }
    return nullptr;
}

/* static */
void
LoggerThread::ensure_initialized() {
    if (This::instance_sf == nullptr) {
        pican::panic("LoggerThread not initialized!");
    }
}

}  // namespace pican::log
