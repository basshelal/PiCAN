#include "pican/Logger.hpp"

namespace pican {
bool Logger::initialized_sf = false;
Logger* Logger::instance_sf = nullptr;

/* static */
bool
Logger::initialize(Count maxEntriesCount, Count maxWritersCount, Level level) {
    if (This::initialized_sf) {
        return true;
    }
    memory::Manager& manager = memory::Manager::get();
    memory::Block loggerBlock = memory::Manager::get().get_block(sizeof(Logger));

    memory::Block entriesBlock = manager.get_block(sizeof(Entry) * maxEntriesCount + 1);
    Array<Entry> entries{entriesBlock};

    memory::Block writersBlock = manager.get_block(sizeof(LogWriter) * maxWritersCount);
    Array<LogWriter> writers{writersBlock};

    This::instance_sf = new (loggerBlock.address_to_ptr<Logger>()) Logger{entries, writers, level};

    This::initialized_sf = true;
    return true;
}

/* static */
void
Logger::register_log_writer(const LogWriter& logWriter) {
    This::ensure_initialized();
    Logger& instance = *This::instance_sf;
    if (instance.writersCount_f >= instance.writers_f.items_count()) {
        return;
    }
    instance.writers_f.set_at(instance.writersCount_f, logWriter);
    ++instance.writersCount_f;
}

}  // namespace pican
