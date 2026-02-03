#include "pican/LoggerThread.hpp"

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

namespace pican {

Thread* LoggerThread::thread_f = nullptr;

/* static */
void
LoggerThread::initialize() {
    if (This::thread_f != nullptr) {
        return;
    }

    memory::Block threadBlock = memory::Manager::get().get_block(sizeof(Thread));

    Thread::Callable<void> loggerThreadRun = [](void* arg) -> void {
        Logger& logger = *Logger::instance_sf;
        while (true) {
            logger.eventfd_f.wait_blocking();

            while (!logger.entries_f.is_empty()) {
                std::optional<Logger::Entry> entryOptional = logger.entries_f.pop_move();
                if (!entryOptional.has_value()) {
                    continue;
                }
                const Logger::Entry& entry = entryOptional.value();

                for (Count i = 0; i < logger.writersCount_f; ++i) {
                    const LogWriter& writer = logger.writers_f.get_at(i);
                    if (writer.level() < entry.level) {
                        continue;
                    }
                    char buffer[Logger::ACTUAL_MESSAGE_ENTRY_SIZE];
                    ::snprintf(buffer,sizeof(buffer),"[%zu] ", entry.timestamp);
                    ::write(writer.file_descriptor(), buffer, strlen(buffer));

                    ::snprintf(buffer,sizeof(buffer),"%s: ", magic_enum::enum_name(entry.level).data());
                    ::write(writer.file_descriptor(), buffer, strlen(buffer));

                    ::snprintf(buffer,sizeof(buffer),"%s\n", entry.message.data());
                    ::write(writer.file_descriptor(), buffer, strlen(buffer));
                }
            }
        }
    };


    This::thread_f =
        new (threadBlock.address_to_ptr<Thread>()) Thread{"LoggerThread", loggerThreadRun, (void*) nullptr};
}

/* static */
void
LoggerThread::start() {
    This::ensure_initialized();
    This::thread_f->start();
}

/* static */
void
LoggerThread::ensure_initialized() {
    if (This::thread_f == nullptr) {
        pican::panic("LoggerThread not initialized!");
    }
}
}  // namespace pican
