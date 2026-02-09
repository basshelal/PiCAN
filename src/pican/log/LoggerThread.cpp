#include "LoggerThread.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "LoggerThread.hpp"
#include "pican/time/DateTime.hpp"

namespace pican::log {
LoggerThread* LoggerThread::instance_sf = nullptr;

LoggerThread::LoggerThread(Level level, Count threadCount, Count sinkCount, Count threadBufferSize) :
    level_f{level}, buffers_f{mem::Manager::get_array<Buffer>(threadCount)}, buffersCount_f{0},
    sinks_f{mem::Manager::get_array<Sink>(sinkCount)}, sinksCount_f{0}, eventfd_f{EventFD::Mode::NOTIFY},
    thread_f{"LoggerThread", &LoggerThread::runnable, this} {
}

/* static */
void
LoggerThread::initialize(Level level, Count threadCount, Count sinkCount, Count threadBufferSize) {
    if (This::instance_sf != nullptr) {
        pican::panic("LoggerThread already initialized!");
    }

    mem::Block block = mem::Manager::get_block(sizeof(LoggerThread));

    This::instance_sf =
        new (block.address_to_ptr<LoggerThread>()) LoggerThread{level, threadCount, sinkCount, threadBufferSize};

    // initialize buffers
    for (Index i = 0; i < This::instance_sf->buffers_f.items_count(); ++i) {
        Array<Entry> array = mem::Manager::get_array<Entry>(threadBufferSize);
        Buffer* buffer = This::instance_sf->buffers_f.get_at_ptr(i);
        buffer = new (buffer) Buffer{array};
    }

    This::ensure_initialized();
}

/* static */
void
LoggerThread::register_logger(const Sink& logger) {
    This::ensure_initialized();
    LoggerThread& instance = *This::instance_sf;
    if (instance.sinksCount_f >= instance.sinks_f.items_count()) {
        pican::panic("Loggers at capacity");
    }
    instance.sinks_f.set_at(instance.sinksCount_f, logger);
    ++instance.sinksCount_f;
}

/* static */
void
LoggerThread::register_thread(ThreadId id) {
    This::ensure_initialized();
    LoggerThread& instance = *This::instance_sf;
    Buffer* found = instance.get_buffer_of_thread(id);
    if (found != nullptr) {
        pican::panic("The thread is already registered!");
    }
    if (instance.buffersCount_f >= instance.buffers_f.items_count()) {
        pican::panic("Buffers at capacity!");
    }

    Buffer* buffer = instance.buffers_f.get_at_ptr(instance.buffersCount_f);
    buffer->threadId_f = id;

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

    const SizeBytes dateTimeBufferSize = time::DateTime::FORMAT_MINIMUM_LENGTH;
    std::array<char, dateTimeBufferSize> dateTimeBuffer;

    const SizeBytes levelBufferSize = LEVEL_STRING_MAX_LENGTH;
    std::array<char, levelBufferSize> levelBuffer;

    const SizeBytes threadBufferSize = 32;
    std::array<char, threadBufferSize> threadBuffer;

    const SizeBytes totalBufferSize =
        dateTimeBufferSize + 1 + levelBufferSize + 1 + threadBufferSize + 1 + MESSAGE_MAX_SIZE;
    std::array<char, totalBufferSize> totalBuffer;

    while (true) {
        instance.eventfd_f.wait_blocking();

        // TODO @basshelal Wed 04-Feb-2026 : This is a pretty good candidate for epoll,
        //  many threads need to "notify" this thread that it needs to wake up and do stuff

        for (Index bufferIndex = 0; bufferIndex < instance.buffersCount_f; ++bufferIndex) {
            Buffer* buffer = instance.buffers_f.get_at_ptr(bufferIndex);
            while (!buffer->entries().is_empty()) {
                for (Index loggerIndex = 0; loggerIndex < instance.sinksCount_f; ++loggerIndex) {
                    Sink* logger = instance.sinks_f.get_at_ptr(loggerIndex);
                    std::optional<Entry> entryOptional = buffer->entries().pop_move();
                    if (!entryOptional.has_value()) {
                        continue;
                    }
                    const Entry& entry = entryOptional.value();

                    if (logger->level() < entry.level()) {
                        continue;
                    }

                    const time::DateTime dateTime = time::DateTime::from_time_point(entry.timestamp());
                    dateTime.format_into(dateTimeBuffer.data(), dateTimeBufferSize);

                    fmt::format_to_n(levelBuffer.data(), levelBufferSize, "{}", level_to_string(entry.level()));

                    const ThreadId threadId = buffer->thread_id();
                    const ThreadName threadName = ThreadManager::get_thread_name_from_id(threadId);
                    fmt::format_to_n(threadBuffer.data(), threadBufferSize, "{}:{}", threadName, threadId);

                    fmt::format_to_n_result<char*> formatted = fmt::format_to_n(
                        totalBuffer.data(), totalBufferSize, "{} {} {} {}\n", dateTimeBuffer.data(), levelBuffer.data(),
                        threadBuffer.data(), entry.message().data()
                    );

                    logger->file_f.write(totalBuffer.data(), sizeof(char), formatted.size);
                }
            }
        }
    }
}

Buffer*
LoggerThread::get_buffer_of_thread(ThreadId threadId) const& {
    if (threadId == 0) {
        return nullptr;
    }
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
