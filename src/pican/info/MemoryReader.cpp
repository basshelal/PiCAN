#include "pican/info/MemoryReader.hpp"

#include <string_view>

#include "pican/Contracts.hpp"
#include "pican/StringSeparator.hpp"
#include "pican/mem/Manager.hpp"

namespace pican::info {

constexpr FilePath MEMINFO_PATH = "/proc/meminfo";
constexpr FilePath SELF_STATUS_PATH = "/proc/self/status";
constexpr SizeBytes FILE_BUFFER_SIZE = 10'000;

namespace {

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

MemoryReader::MemoryReader(pican::File&& memInfoFile, pican::File&& selfStatusFile, const mem::Block& fileBuffer) :
    memInfoFile_f{std::move(memInfoFile)}, selfStatusFile_f{std::move(selfStatusFile)}, fileBuffer_f{fileBuffer} {
    CONTRACTS_ASSERT(this->memInfoFile_f.is_open());
    CONTRACTS_ASSERT(this->selfStatusFile_f.is_open());
}

SimpleResult<MemoryReader::Error>
MemoryReader::update_info() & {
    const SimpleResult<Error> memInfoReadResult = this->read_meminfo_file();
    if (memInfoReadResult.is_failure()) {
        return memInfoReadResult;
    }
    const SimpleResult<Error> selfStatusReadResult = this->read_self_status_file();
    if (selfStatusReadResult.is_failure()) {
        return selfStatusReadResult;
    }

    return SimpleResult<MemoryReader::Error>::success_default();
}

SizeBytes
MemoryReader::get_total_system_memory() const& {
    TODO_NOT_IMPLEMENTED();
    // return read_entry_from_file(this->memInfoFile_f, "MemTotal");
}

SizeBytes
MemoryReader::get_free_system_memory() const& {
    TODO_NOT_IMPLEMENTED();
    // return read_entry_from_file(this->memInfoFile_f, "MemAvailable");
}

SizeBytes
MemoryReader::get_used_system_memory() const& {
    const SizeBytes total = this->get_total_system_memory();
    const SizeBytes free = this->get_free_system_memory();
    return total - free;
}

SizeBytes
MemoryReader::get_process_used_memory() const& {
    TODO_NOT_IMPLEMENTED();
    // return read_entry_from_file(this->selfStatusFile_f, "VmRSS");
}

SimpleResult<MemoryReader::Error>
MemoryReader::read_meminfo_file() & {
    this->memInfoFile_f.seek_to(0);

    const Result<SizeBytes, File::Error> readResult = this->memInfoFile_f.read_into(this->fileBuffer_f);
    if (readResult.is_failure()) {
        return SimpleResult<MemoryReader::Error>::failure_by_copy(MemoryReader::Error::FILE_READ_ERROR);
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
            this->info_f.totalMemory = parsed * 1'024;
            fprintf(stderr, "MEM TOTAL: %zu\n", this->info_f.totalMemory);
        }

        print_string_view(stderr, entryName);
        fprintf(stderr, "%s", "\t\t\t\t\t");
        print_string_view(stderr, entryValue);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    return SimpleResult<MemoryReader::Error>::success_default();
}

SimpleResult<MemoryReader::Error>
MemoryReader::read_self_status_file() & {
    this->selfStatusFile_f.seek_to(0);

    const Result<SizeBytes, File::Error> readResult = this->selfStatusFile_f.read_into(this->fileBuffer_f);
    if (readResult.is_failure()) {
        return SimpleResult<MemoryReader::Error>::failure_by_copy(MemoryReader::Error::FILE_READ_ERROR);
    }
    const SizeBytes bytesRead = readResult.success_value_or_panic();
    CONTRACTS_ASSERT(bytesRead < this->fileBuffer_f.size_bytes());  // we read the file to its end

    StringSeparator lineIterator{this->fileBuffer_f.address_to_ptr<char>(), bytesRead, '\n'};
    while (lineIterator.has_next()) {
        const std::string_view line = lineIterator.next();
        const std::string_view entryName = entry_name(line);
        const std::string_view entryValue = entry_value(line);

        // Read documentation here:
        // https://man7.org/linux/man-pages/man5/proc_pid_status.5.html
        print_string_view(stderr, entryName);
        fprintf(stderr, "%s", "\t\t\t\t\t");
        print_string_view(stderr, entryValue);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    return SimpleResult<MemoryReader::Error>::success_default();
}

pican::Result<MemoryReader, MemoryReader::Error>
MemoryReader::create() {
    CONTRACTS_ASSERT(File::exists(MEMINFO_PATH));
    CONTRACTS_ASSERT(File::exists(SELF_STATUS_PATH));

    pican::File memInfoFile{MEMINFO_PATH};

    const File::SimpleResult memInfoOpenResult = memInfoFile.open(FileMode::READ_ONLY, false);
    if (memInfoOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    pican::File selfStatusFile{SELF_STATUS_PATH};

    const File::SimpleResult selfStatusOpenResult = selfStatusFile.open(FileMode::READ_ONLY, false);
    if (selfStatusOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    mem::Block fileBuffer = mem::Manager::get_block(FILE_BUFFER_SIZE);

    return pican::Result<MemoryReader, MemoryReader::Error>::success_by_move(
        MemoryReader{std::move(memInfoFile), std::move(selfStatusFile), fileBuffer}
    );
}

}  // namespace pican::info
