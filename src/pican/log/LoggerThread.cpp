#include "pican/log/LoggerThread.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <pican/Config.hpp>
#include <pican/EventFD.hpp>
#include <unistd.h>

#include "pican/time/DateTime.hpp"

namespace pican::log {

LoggerThread::LoggerThread(log::Level level, ThreadName name, Array<log::Buffer> buffers, Array<log::Sink> sinks) :
    level_f{level}, buffers_f{buffers}, buffersCount_f{0}, sinks_f{sinks}, eventfd_f{EventFD::Mode::NOTIFY},
    thread_f{name, &LoggerThread::runnable, this}, counter_f{0}, identity_f{-1, name} {
}

void
LoggerThread::start() & {
    if (this->thread_f.state() == ThreadState::RUNNING) {
        return;
    }
    this->thread_f.start();
}

void
LoggerThread::stop() & {
    if (this->thread_f.state() == ThreadState::STOPPED) {
        return;
    }
    this->thread_f.stop();
}

ThreadState
LoggerThread::thread_state() const& {
    return this->thread_f.state();
}

ThreadCounterValue
LoggerThread::thread_counter_value() const& {
    return this->counter_f.load(std::memory_order_relaxed);
}

const ThreadIdentity&
LoggerThread::thread_identity() const& {
    return this->identity_f;
}

LoggerThread::Result
LoggerThread::register_sink(const Sink& sink) & {
    if (this->sinks_f.size() >= this->sinks_f.capacity()) {
        return LoggerThread::Result::failure_by_copy(LoggerThread::Error::CAPACITY_REACHED);
    }
    this->sinks_f.add_copy(sink);
    return LoggerThread::Result::success_default();
}

LoggerThread::Result
LoggerThread::register_thread(const ThreadIdentity& threadIdentity) & {
    Buffer* found = this->get_buffer_of_thread(threadIdentity.id);
    if (found != nullptr) {
        return LoggerThread::Result::failure_by_copy(LoggerThread::Error::ALREADY_REGISTERED);
    }
    if (this->buffersCount_f >= this->buffers_f.length()) {
        return LoggerThread::Result::failure_by_copy(LoggerThread::Error::CAPACITY_REACHED);
    }

    Buffer& buffer = this->buffers_f.get(this->buffersCount_f);
    buffer.threadIdentity_f = threadIdentity;
    ++this->buffersCount_f;
    return LoggerThread::Result::success_default();
}

void
LoggerThread::log_entry(const Entry& entry) & {
    if (entry.level() > this->level_f) {
        return;
    }

    const ThreadId currentThreadId = pican::Thread::calling_thread_id();

    Buffer* foundBuffer = this->get_buffer_of_thread(currentThreadId);
    if (foundBuffer == nullptr) {
        return;
        // no buffer for the calling thread
    }

    foundBuffer->entries_f.push_copy(entry);
    this->eventfd_f.notify();
}

/* static */
pican::Result<LoggerThread*, LoggerThread::Error>
LoggerThread::create(mem::Block block, log::Level level, ThreadName name, Count sinkCount, Count bufferEntryCount) {
    CONTRACTS_PRECONDITION(block.size_bytes() >= sizeof(LoggerThread));

    Array<Buffer> buffers = mem::Manager::get_array<Buffer>(config::THREADS_COUNT);
    // initialize buffers
    for (Index i = 0; i < buffers.length(); ++i) {
        Array<Entry> entries = mem::Manager::get_array<Entry>(bufferEntryCount);
        Buffer* buffer = buffers.get_ptr(i);
        buffer = new (buffer) Buffer{entries};
    }
    Array<Sink> sinks = mem::Manager::get_array<Sink>(sinkCount);

    LoggerThread* thread = new (block.address_to_ptr<LoggerThread>()) LoggerThread{level, name, buffers, sinks};

    CONTRACTS_ASSERT(thread != nullptr);

    return pican::Result<LoggerThread*, LoggerThread::Error>::success_by_copy(thread);
}

Buffer*
LoggerThread::get_buffer_of_thread(const ThreadId& id) const& {
    for (Buffer& buffer : this->buffers_f) {
        if (buffer.threadIdentity_f.id == id) {
            return &buffer;
        }
    }
    return nullptr;
}

/* static */
void
LoggerThread::runnable(LoggerThread* self) {
    const SizeBytes dateTimeBufferSize = time::DateTime::FORMAT_MINIMUM_LENGTH;
    std::array<char, dateTimeBufferSize> dateTimeBuffer;

    const SizeBytes levelBufferSize = LEVEL_STRING_MAX_LENGTH;
    const SizeBytes threadBufferSize = 32;

    const SizeBytes totalBufferSize =
        dateTimeBufferSize + 1 + levelBufferSize + 1 + threadBufferSize + 1 + MESSAGE_MAX_SIZE;
    std::array<char, totalBufferSize> totalBuffer;

    while (true) {
        self->counter_f.atomic().fetch_add(1, std::memory_order_relaxed);
        self->eventfd_f.wait_blocking();

        for (Buffer& buffer : self->buffers_f) {
            Count toPop = buffer.entries_f.size();

            while (toPop > 0) {
                std::optional<Entry> entryOptional = buffer.entries_f.pop_move();
                --toPop;
                if (!entryOptional.has_value()) {
                    continue;
                }
                const Entry& entry = entryOptional.value();

                for (Sink& sink : self->sinks_f) {
                    if (sink.level() < entry.level()) {
                        continue;
                    }

                    const time::DateTime dateTime = time::DateTime::from_time_point(entry.timestamp());
                    dateTime.format_into(dateTimeBuffer.data(), dateTimeBufferSize);

                    const ThreadIdentity threadIdentity = buffer.thread_identity();

                    fmt::format_to_n_result<char*> formatted = fmt::format_to_n(
                        totalBuffer.data(), totalBufferSize, "{} {} {}:{} {}\n", dateTimeBuffer.data(),
                        level_to_string(entry.level()), threadIdentity.name, threadIdentity.id, entry.message_buffer()
                    );

                    sink.file_f.write(totalBuffer.data(), sizeof(char), formatted.size);
                }
            }
        }
    }
}

}  // namespace pican::log
