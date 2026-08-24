module;

#include <atomic>
#include <cstdint>
#include <optional>

#include "pican/contracts.hpp"

export module pican.log:LoggerThread;

import fmt;

import :Sink;
import :Buffer;
import :Entry;
import :utils;

import pican.core;
import pican.mem;
import pican.ds;
import pican.fs;
import pican.sync;
import pican.time;

export namespace pican::log {
//
// class LoggerThread : public IApplicationThread {
// public:  // types
//     enum class Error : std::uint8_t {
//         ALREADY_REGISTERED,
//         CAPACITY_REACHED,
//     };
//
//     using Result = pican::SimpleResult<Error>;
//
// private:  // types
//
// private:               // fields
//     LogLevel level_f;  // TODO @basshelal Tue 28-Jul-2026 : Make this dynamically settable
//     ds::Map<ThreadId, ds::RingBuffer<Entry>> entriesMap_f;
//     ds::ArrayList<log::Sink> sinks_f;
//     sync::EventFd eventfd_f;
//     Thread thread_f;
//     ThreadCounter counter_f;
//     CopyableAtomic<bool> isRunning_f;
//
// private:  // constructor
//     LoggerThread(LogLevel level, ThreadName name, ds::Array<log::Buffer> buffers, ds::Array<log::Sink> sinks) :
//         level_f{level}, buffers_f{buffers}, buffersCount_f{0}, sinks_f{sinks},
//         eventfd_f{sync::EventFd::Mode::NOTIFY}, thread_f{name, &LoggerThread::runnable, this}, counter_f{0},
//         isRunning_f{false} {
//     }
//
// public:  // lifetime
//     LoggerThread() = delete;
//
//     LoggerThread(const LoggerThread& rhs) = delete;
//
//     LoggerThread(LoggerThread&& rhs) = default;
//
//     LoggerThread&
//     operator=(const LoggerThread& rhs) & = delete;
//
//     LoggerThread&
//     operator=(LoggerThread&& rhs) & = default;
//
//     ~LoggerThread() override = default;
//
// public:  // named constructors
//     [[nodiscard]]
//     static pican::Result<LoggerThread*, Error>
//     create(mem::Block block, LogLevel level, ThreadName name, Count sinkCount, Count bufferEntryCount) {
//         CONTRACTS_PRECONDITION(block.size_bytes() >= sizeof(LoggerThread));
//
//         const SizeBytes threadEntriesMapBlockSize =
//             decltype(LoggerThread::entriesMap_f)::ELEMENT_SIZE * bufferEntryCount;
//         mem::Block threadEntriesMapBlock = mem::Manager::get_block();
//
//
//         const SizeBytes sizeofEntries = sizeof(Entry) * bufferEntryCount;
//         mem::Block entriesNullBlock = mem::Block::from_address(mem::NULL_ADDRESS, sizeofEntries);
//         ds::Array<Entry> entriesNullArray{entriesNullBlock, Entry{level}};
//         Buffer defaultBuffer{entriesNullArray};
//         // default buffer which needs a default array of entryCount entries
//
//
//         const SizeBytes bufferBlockSize = sizeof(Buffer) * bufferEntryCount;
//         mem::Block nullBufferNullBlock{mem::NULL_ADDRESS, bufferBlockSize};
//         Buffer nullBuffer{ds::Array<Buffer>{nullBufferNullBlock}};
//         ds::Array<Buffer> buffers = mem::Manager::get_array<Buffer>(config::THREADS_COUNT, defaultBuffer);
//         // initialize buffers
//         for (Index i = 0; i < buffers.length(); ++i) {
//             ds::Array<Entry> entries = mem::Manager::get_array<Entry>(bufferEntryCount);
//             Buffer* buffer = buffers.get_ptr(i);
//             buffer = new (buffer) Buffer{entries};
//         }
//         ds::Array<Sink> sinks = mem::Manager::get_array<Sink>(sinkCount);
//
//         LoggerThread* thread = new (block.address_to_ptr<LoggerThread>()) LoggerThread{level, name, buffers, sinks};
//
//         CONTRACTS_ASSERT(thread != nullptr);
//
//         return pican::Result<LoggerThread*, LoggerThread::Error>::success_by_copy(thread);
//     }
//
// private:  // member functions
//     [[nodiscard]]
//     Buffer*
//     get_buffer_of_thread(const ThreadId& id) const& {
//         for (Buffer& buffer : this->buffers_f) {
//             if (buffer.threadIdentity_f.id == id) {
//                 return &buffer;
//             }
//         }
//         return nullptr;
//     }
//
// private:  // static functions
//     void
//     log_function(LogLevel level, fmt::string_view format, fmt::format_args args) {
//     }
//
//     static_assert(std::is_same_v<decltype(&log_function), LogFunctionPtr>);
//
//     static void
//     runnable(LoggerThread* self) {
//         const SizeBytes dateTimeBufferSize = time::DateTime::FORMAT_MINIMUM_LENGTH;
//         std::array<char, dateTimeBufferSize> dateTimeBuffer;
//
//         const SizeBytes levelBufferSize = LEVEL_STRING_MAX_LENGTH;
//         const SizeBytes threadBufferSize = 32;
//
//         const SizeBytes totalBufferSize =
//             dateTimeBufferSize + 1 + levelBufferSize + 1 + threadBufferSize + 1 + MESSAGE_MAX_SIZE;
//         std::array<char, totalBufferSize> totalBuffer;
//
//         while (self->isRunning_f.load(std::memory_order_acquire)) {
//             self->counter_f.atomic().fetch_add(1, std::memory_order_acq_rel);
//             self->eventfd_f.wait_blocking();
//             if (!self->isRunning_f.load(std::memory_order_acquire)) {
//                 break;
//             }
//
//             for (Count i = 0; i < self->buffersCount_f; ++i) {
//                 Buffer& buffer = self->buffers_f[i];
//                 Count toPop = buffer.entries_f.size();
//
//                 while (toPop > 0) {
//                     std::optional<Entry> entryOptional = buffer.entries_f.pop_move();
//                     --toPop;
//                     if (!entryOptional.has_value()) {
//                         continue;
//                     }
//                     const Entry& entry = entryOptional.value();
//
//                     for (Sink& sink : self->sinks_f) {
//                         if (sink.level() < entry.level()) {
//                             continue;
//                         }
//
//                         const time::DateTime dateTime = time::DateTime::from_time_point(entry.timestamp());
//                         dateTime.format_into(dateTimeBuffer.data(), dateTimeBufferSize);
//
//                         const ThreadIdentity& threadIdentity = buffer.thread_identity();
//
//                         // TODO @basshelal Fri 20-Feb-2026 : Document the text limits in code somehow,
//                         //  Levels are 4 at most VERBOSE and ERROR will be automatically truncated
//                         //  Thread names are 4 at most because we can, longest will be "main"
//                         //  Thread IDs will be at most 8 digits, though I think we can get away with way less
//                         fmt::format_to_n_result<char*> formatted = fmt::format_to_n(
//                             totalBuffer.data(), totalBufferSize, "{} {:<4.4} {:<4}:{:<8} {}\n",
//                             dateTimeBuffer.data(), log_level_to_string(entry.level()), threadIdentity.name,
//                             threadIdentity.id, entry.message_buffer()
//                         );
//
//                         sink.file_f.write_from(totalBuffer.data(), formatted.size);
//                     }
//                 }
//             }
//         }
//     }
//
// public:  // member functions
//     virtual ThreadState
//         start() &
//         override {
//         if (this->thread_state() == ThreadState::RUNNING) {
//             return this->thread_state();
//         }
//         this->isRunning_f.store(true, std::memory_order_release);
//         this->thread_f.start();
//         return this->thread_state();
//     }
//
//     virtual ThreadState
//         stop() &
//         override {
//         if (this->thread_f.state() != ThreadState::RUNNING) {
//             return this->thread_state();
//         }
//         this->isRunning_f.store(true, std::memory_order_release);
//         this->eventfd_f.notify();
//         return this->thread_state();
//     }
//
//     [[nodiscard]]
//     virtual ThreadCounterValue
//     counter_value() const& override {
//         return this->counter_f.load(std::memory_order_acquire);
//     }
//
//     [[nodiscard]]
//     virtual const Thread&
//     backing_thread() const& override {
//         return this->thread_f;
//     }
//
//     [[nodiscard]]
//     LoggerThread::Result
//     register_sink(Sink&& sink) & {
//         if (this->sinks_f.size() >= this->sinks_f.capacity()) {
//             return LoggerThread::Result::failure_by_copy(LoggerThread::Error::CAPACITY_REACHED);
//         }
//         this->sinks_f.add_move(std::move(sink));
//         return LoggerThread::Result::success_default();
//     }
//
//     [[nodiscard]]
//     LoggerThread::Result
//     register_thread(const ThreadIdentity& threadIdentity) & {
//         Buffer* found = this->get_buffer_of_thread(threadIdentity.id);
//         if (found != nullptr) {
//             return LoggerThread::Result::failure_by_copy(LoggerThread::Error::ALREADY_REGISTERED);
//         }
//         if (this->buffersCount_f >= this->buffers_f.length()) {
//             return LoggerThread::Result::failure_by_copy(LoggerThread::Error::CAPACITY_REACHED);
//         }
//
//         Buffer& buffer = this->buffers_f.get(this->buffersCount_f);
//         buffer.threadIdentity_f = threadIdentity;
//         ++this->buffersCount_f;
//         return LoggerThread::Result::success_default();
//     }
//
//     void
//     log_entry(const Entry& entry) & {
//         if (entry.level() > this->level_f) {
//             return;
//         }
//
//         const ThreadId currentThreadId = pican::Thread::calling_thread_id();
//
//         Buffer* foundBuffer = this->get_buffer_of_thread(currentThreadId);
//         if (foundBuffer == nullptr) {
//             return;
//             // no buffer for the calling thread
//         }
//
//         foundBuffer->entries_f.push_copy(entry);
//         this->eventfd_f.notify();
//     }
// };
//
}  // namespace pican::log
