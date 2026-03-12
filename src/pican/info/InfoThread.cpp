#include "pican/info/InfoThread.hpp"

#include <pican/EventFD.hpp>
#include <pican/Log.hpp>

#include "pican/ApplicationState.hpp"
#include "pican/Contracts.hpp"
#include "pican/StringSeparator.hpp"

namespace pican::info {

namespace {
constexpr FilePath MEMINFO_PATH = "/proc/meminfo";
constexpr FilePath STATUS_PATH = "/proc/self/status";
constexpr SizeBytes FILE_BUFFER_SIZE = 10'000;

[[maybe_unused]]
void
print_string_view(FILE* file, std::string_view string) {
    for (Index i = 0; i < string.length(); ++i) {
        fprintf(file, "%c", string.at(i));
    }
}

[[nodiscard]]
std::string_view
entry_name(std::string_view line) {
    const char* ptr = line.data();
    Index end = line.length() - 1;
    for (Index i = 0; i < line.length(); ++i) {
        char c = *(ptr + i);
        if (c == ':') {
            end = i;
            break;
        }
    }
    return std::string_view{ptr, end};
}

[[nodiscard]]
std::string_view
entry_value(std::string_view line) {
    const char* ptr = line.data();
    Index start = 0;
    bool foundColon = false;
    for (Index i = 0; i < line.length(); ++i) {
        char c = *(ptr + i);
        if (foundColon && !::isspace(c)) {
            start = i;
            break;
        } else if (!foundColon && c == ':') {
            foundColon = true;
        }
    }
    return std::string_view{ptr + start, line.length() - start};
}

[[nodiscard]]
std::uint64_t
string_to_uint64(std::string_view str) {
    return ::strtoull(str.data(), nullptr, 10);
}

}  // namespace

InfoThread::InfoThread(
    Milliseconds sleepMillis, mem::Block fileBuffer, pican::File&& meminfoFile, pican::File&& statusFile
) :
    thread_f{"info", &InfoThread::runnable, this}, isRunning_f{false}, counter_f{0}, sleepMillis_f{sleepMillis},
    fileBuffer_f{std::move(fileBuffer)}, memInfoFile_f{std::move(meminfoFile)}, statusFile_f{std::move(statusFile)},
    stopFd_f{EventFD::Mode::NOTIFY} {
    CONTRACTS_ASSERT(this->memInfoFile_f.is_open());
    CONTRACTS_ASSERT(this->statusFile_f.is_open());
}

ThreadState
InfoThread::start() & {
    if (this->thread_state() == ThreadState::RUNNING) {
        return this->thread_state();
    }
    this->isRunning_f.store(true, std::memory_order_release);
    this->thread_f.start();
    return this->thread_state();
}

ThreadState
InfoThread::stop() & {
    if (this->thread_f.state() != ThreadState::RUNNING) {
        return this->thread_state();
    }
    this->isRunning_f.store(true, std::memory_order_release);
    this->stopFd_f.notify();
    return this->thread_state();
}

ThreadCounterValue
InfoThread::counter_value() const& {
    return this->counter_f.load(std::memory_order_acquire);
}

const Thread&
InfoThread::backing_thread() const& {
    return this->thread_f;
}

SimpleResult<InfoThread::Error>
InfoThread::read_meminfo_file() & {
    this->memInfoFile_f.seek_to(0);

    const Result<SizeBytes, File::Error> readResult = this->memInfoFile_f.read_into(this->fileBuffer_f);
    if (readResult.is_failure()) {
        return SimpleResult<InfoThread::Error>::failure_by_copy(InfoThread::Error::FILE_READ_ERROR);
    }
    const SizeBytes bytesRead = readResult.success_value_or_panic();
    CONTRACTS_ASSERT(bytesRead < this->fileBuffer_f.size_bytes());  // we read the file to its end

    StringSeparator separator{this->fileBuffer_f.address_to_ptr<char>(), bytesRead, '\n'};
    while (separator.has_next()) {
        const std::string_view line = separator.next();
        const std::string_view entryName = entry_name(line);
        const std::string_view entryValue = entry_value(line);

        // Read documentation here:
        // https://man7.org/linux/man-pages/man5/proc_meminfo.5.html
        if (entryName == "MemTotal") {
            const SizeBytes parsed = string_to_uint64(entryValue);
            SizeBytes totalMemory = parsed * 1'024;
            pican::log_info("MEM TOTAL: {}", totalMemory);
        }
    }

    return SimpleResult<InfoThread::Error>::success_default();
}

SimpleResult<InfoThread::Error>
InfoThread::read_status_file() & {
    this->statusFile_f.seek_to(0);

    const Result<SizeBytes, File::Error> readResult = this->statusFile_f.read_into(this->fileBuffer_f);
    if (readResult.is_failure()) {
        return SimpleResult<InfoThread::Error>::failure_by_copy(InfoThread::Error::FILE_READ_ERROR);
    }
    const SizeBytes bytesRead = readResult.success_value_or_panic();
    CONTRACTS_ASSERT(bytesRead < this->fileBuffer_f.size_bytes());  // we read the file to its end

    StringSeparator lineIterator{this->fileBuffer_f.address_to_ptr<char>(), bytesRead, '\n'};
    while (lineIterator.has_next()) {
        const std::string_view line = lineIterator.next();
        [[maybe_unused]]
        const std::string_view entryName = entry_name(line);
        [[maybe_unused]]
        const std::string_view entryValue = entry_value(line);
        // Read documentation here:
        // https://man7.org/linux/man-pages/man5/proc_pid_status.5.html
    }

    return SimpleResult<InfoThread::Error>::success_default();
}

/* static */
pican::Result<InfoThread*, InfoThread::Error>
InfoThread::create(mem::Block block, mem::Block fileBuffer, Milliseconds sleepMillis) {
    using Ret = pican::Result<InfoThread*, InfoThread::Error>;
    CONTRACTS_PRECONDITION(block.size_bytes() >= sizeof(InfoThread));

    CONTRACTS_ASSERT(File::exists(MEMINFO_PATH));
    CONTRACTS_ASSERT(File::exists(STATUS_PATH));

    pican::File memInfoFile{MEMINFO_PATH};
    const File::SimpleResult memInfoOpenResult = memInfoFile.open(FileMode::READ_ONLY, false);
    if (memInfoOpenResult.is_failure()) {
        return Ret::failure_by_copy(InfoThread::Error::FAILED_TO_OPEN_FILE);
    }

    pican::File statusFile{STATUS_PATH};
    const File::SimpleResult selfStatusOpenResult = statusFile.open(FileMode::READ_ONLY, false);
    if (selfStatusOpenResult.is_failure()) {
        return Ret::failure_by_copy(InfoThread::Error::FAILED_TO_OPEN_FILE);
    }

    InfoThread* infoThreadPtr = new (block.address_to_ptr<InfoThread>())
        InfoThread{sleepMillis, fileBuffer, std::move(memInfoFile), std::move(statusFile)};

    CONTRACTS_ASSERT(infoThreadPtr != nullptr);

    return Ret::success_by_copy(infoThreadPtr);
}

void
InfoThread::runnable(InfoThread* self) {
    while (self->isRunning_f.load(std::memory_order_acquire)) {
        self->counter_f.atomic().fetch_add(1, std::memory_order_acq_rel);
        // self->stopFd_f.wait_blocking();
        ::usleep(self->sleepMillis_f * 1'000);
        if (!self->isRunning_f.load(std::memory_order_acquire)) {
            break;
        }

        self->read_meminfo_file();
        self->read_status_file();

        const can::CanInfo canInfo = ApplicationState::get().canInfo_f.read();
        pican::log_info("lastFrameWaitTime: {}", canInfo.lastFrameWaitTime);
        pican::log_info("lastFrameProcessingTime: {}", canInfo.lastFrameProcessingTime);
        pican::log_info("lastFrameSize: {}", canInfo.lastFrameSize);
    }
}

}  // namespace pican::info
