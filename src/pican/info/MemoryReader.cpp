#include "pican/info/MemoryReader.hpp"

#include <string_view>

#include "pican/Contracts.hpp"

namespace pican::info {

constexpr FilePath MEMINFO_PATH = "/proc/meminfo";
constexpr FilePath SELF_STATUS_PATH = "/proc/self/status";

namespace {

SizeBytes
read_line(File& file, char* buffer, std::size_t bufferSize) {
    const Result<SizeBytes, File::Error> read = file.read_into(buffer, bufferSize);

    if (read.is_failure()) {
        return 0;
    }

    for (Index i = 0; i < bufferSize; ++i) {
        char c = buffer[i];
        if (c == '\n' || c == '\0') {
            return i;
        }
    }
    return bufferSize;
}

[[nodiscard]]
SizeBytes
read_entry_from_file(File& file, std::string_view entry) {
    const std::size_t entryLength = entry.length();

    const std::size_t bufferSize = 128;
    char buffer[bufferSize];

    // TODO @basshelal Tue 24-Feb-2026 : CONTINUE HERE!!!

    const auto result = file.total_size_bytes();
    const Offset endOffset = result.success_value_or_panic();
    Offset offset = 0;
    SizeBytes readBytes = 0;
    SizeBytes resultKB = 0;
    do {
        file.seek_to(offset);
        // read file line by line, memory files are line by line
        readBytes = read_line(file, buffer, bufferSize);

        fprintf(stderr, "%s\n", buffer);
        fprintf(stderr, "%zu -> %zu\n", offset, endOffset);

        bool lineFound = ::strncmp(buffer, entry.data(), entryLength) == 0;
        if (lineFound) {
            char* c = buffer + entryLength;
            while (c != nullptr && !::isdigit(*c)) {
                c++;
            }
            resultKB = ::strtoull(c, nullptr, 10);
            break;
        }

        offset += readBytes + 1;
    } while (readBytes != 0 || offset < endOffset);

    return resultKB * 1'024;
}
}  // namespace

MemoryReader::MemoryReader(pican::File&& memInfoFile, pican::File&& selfStatusFile) :
    memInfoFile_f{std::move(memInfoFile)}, selfStatusFile_f{std::move(selfStatusFile)} {
    CONTRACTS_ASSERT(this->memInfoFile_f.is_open());
    CONTRACTS_ASSERT(this->selfStatusFile_f.is_open());
}

SizeBytes
MemoryReader::get_total_system_memory() const& {
    return read_entry_from_file(this->memInfoFile_f, "MemTotal");
}

SizeBytes
MemoryReader::get_free_system_memory() const& {
    return read_entry_from_file(this->memInfoFile_f, "MemAvailable");
}

SizeBytes
MemoryReader::get_used_system_memory() const& {
    const SizeBytes total = this->get_total_system_memory();
    const SizeBytes free = this->get_free_system_memory();
    return total - free;
}

SizeBytes
MemoryReader::get_process_used_memory() const& {
    return read_entry_from_file(this->selfStatusFile_f, "VmRSS");
}

pican::Result<MemoryReader, MemoryReader::Error>
MemoryReader::create() {
    CONTRACTS_ASSERT(File::exists(MEMINFO_PATH));
    CONTRACTS_ASSERT(File::exists(SELF_STATUS_PATH));

    pican::File memInfoFile{MEMINFO_PATH};
    pican::File selfStatusFile{SELF_STATUS_PATH};

    const File::SimpleResult memInfoOpenResult = memInfoFile.open(FileMode::READ_ONLY);

    if (memInfoOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    const File::SimpleResult selfStatusOpenResult = selfStatusFile.open(FileMode::READ_ONLY);

    if (selfStatusOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    return pican::Result<MemoryReader, MemoryReader::Error>::success_by_move(
        MemoryReader{std::move(memInfoFile), std::move(selfStatusFile)}
    );
}

}  // namespace pican::info
