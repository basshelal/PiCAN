#include "pican/log/LoggerThread.hpp"

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "pican/time/DateTime.hpp"

namespace pican::log {

LoggerThread::LoggerThread(
    log::Level level, Count sinkCount, Count threadCount, Count threadBufferSize, ThreadName name
) :
    level_f{level}, buffers_f{mem::Manager::get_array<Buffer>(threadCount)},
    sinks_f{mem::Manager::get_array<Sink>(sinkCount)}, eventfd_f{EventFD::Mode::NOTIFY},
    thread_f{&LoggerThread::runnable, this}, counter_f{0}, identity_f{-1, name} {
    // initialize buffers
    for (Index i = 0; i < this->buffers_f.capacity(); ++i) {
        Array<Entry> array = mem::Manager::get_array<Entry>(threadBufferSize);
        Buffer* buffer = this->buffers_f.get_ptr(i);
        buffer = new (buffer) Buffer{array};
    }
}

/* static */
void
LoggerThread::runnable(LoggerThread* self) {
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
        self->counter_f.fetch_add(1, std::memory_order_relaxed);
        self->eventfd_f.wait_blocking();
        for (Buffer& buffer : self->buffers_f) {
            // we will pop this many entries only, any entries added to this buffer
            //  while we are printing will be processed on the next run, this is to avoid starvation of one
            //  buffer by a very busy/full buffer
            Count toPop = buffer.entries_f.size();
            while (toPop > 0) {
                for (Sink& sink : self->sinks_f) {
                    std::optional<Entry> entryOptional = buffer.entries_f.pop_move();
                    --toPop;
                    if (!entryOptional.has_value()) {
                        continue;
                    }
                    const Entry& entry = entryOptional.value();

                    if (sink.level() < entry.level()) {
                        continue;
                    }

                    const time::DateTime dateTime = time::DateTime::from_time_point(entry.timestamp());
                    dateTime.format_into(dateTimeBuffer.data(), dateTimeBufferSize);

                    fmt::format_to_n(levelBuffer.data(), levelBufferSize, "{}", level_to_string(entry.level()));

                    const ThreadIdentity threadIdentity = buffer.thread_identity();
                    fmt::format_to_n(
                        threadBuffer.data(), threadBufferSize, "{}:{}", threadIdentity.name, threadIdentity.id
                    );

                    fmt::format_to_n_result<char*> formatted = fmt::format_to_n(
                        totalBuffer.data(), totalBufferSize, "{} {} {} {}\n\0", dateTimeBuffer.data(),
                        levelBuffer.data(), threadBuffer.data(), entry.message().data()
                    );

                    sink.file_f.write(totalBuffer.data(), sizeof(char), formatted.size);
                }
            }
        }
    }
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
LoggerThread::register_thread(const ThreadIdentity& identity) & {
    Buffer* found = this->get_buffer_of_thread(identity.id);
    if (found != nullptr) {
        return LoggerThread::Result::failure_by_copy(LoggerThread::Error::ALREADY_REGISTERED);
    }
    if (this->buffers_f.size() >= this->buffers_f.capacity()) {
        return LoggerThread::Result::failure_by_copy(LoggerThread::Error::CAPACITY_REACHED);
    }

    Buffer& buffer = this->buffers_f.get(this->buffers_f.size());
    buffer.threadIdentity_f = identity;
    return LoggerThread::Result::success_default();
}

void
LoggerThread::start() & {
    if (this->thread_f.state() == ThreadState::RUNNING) {
        return;
    }
    this->thread_f.start();
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

}  // namespace pican::log
