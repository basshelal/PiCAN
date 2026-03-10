#include "pican/info/MemoryReader.hpp"

#include <string_view>

#include "pican/Contracts.hpp"
#include "pican/mem/Manager.hpp"

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

[[maybe_unused]] [[nodiscard]]
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

MemoryReader::MemoryReader(pican::File&& memInfoFile, pican::File&& selfStatusFile, const mem::Block& lineBuffer) :
    memInfoFile_f{std::move(memInfoFile)}, selfStatusFile_f{std::move(selfStatusFile)}, lineBuffer_f{lineBuffer} {
    CONTRACTS_ASSERT(this->memInfoFile_f.is_open());
    CONTRACTS_ASSERT(this->memInfoFile_f.has_read_buffer());
    CONTRACTS_ASSERT(this->selfStatusFile_f.is_open());
    CONTRACTS_ASSERT(this->selfStatusFile_f.has_read_buffer());
}

SimpleResult<MemoryReader::Error>
MemoryReader::update_info() & {
    const SimpleResult<Error> memInfoReadResult = this->read_meminfo_file();
    if (memInfoReadResult.is_failure()) {
        return memInfoReadResult;
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

    SizeBytes bytesRead = 1;
    while (bytesRead > 0) {
        const Result<SizeBytes, File::Error> readResult = this->memInfoFile_f.read_into(this->lineBuffer_f);
        if (readResult.is_failure()) {
            return SimpleResult<MemoryReader::Error>::failure_by_copy(MemoryReader::Error::FILE_READ_ERROR);
        }
        bytesRead = readResult.success_value_or_panic();
        const std::string_view readString{this->lineBuffer_f.address_to_ptr<const char>(), bytesRead};

        fprintf(stderr, "%s", readString.data());
        fflush(stderr);
    }
    return SimpleResult<MemoryReader::Error>::success_default();
}

pican::Result<MemoryReader, MemoryReader::Error>
MemoryReader::create() {
    CONTRACTS_ASSERT(File::exists(MEMINFO_PATH));
    CONTRACTS_ASSERT(File::exists(SELF_STATUS_PATH));

    pican::File memInfoFile{MEMINFO_PATH};
    mem::Block memInfoFileReadBuffer = mem::Manager::get_block(4096);

    const File::SimpleResult memInfoSetBufferResult = memInfoFile.set_read_buffer(memInfoFileReadBuffer);
    if (memInfoSetBufferResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_SET_BUFFER
        );
    }

    const File::SimpleResult memInfoOpenResult = memInfoFile.open(FileMode::READ_ONLY, false);
    if (memInfoOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    pican::File selfStatusFile{SELF_STATUS_PATH};
    mem::Block selfStatusFileReadBuffer = mem::Manager::get_block(4096);

    const File::SimpleResult selfStatusSetBufferResult = selfStatusFile.set_read_buffer(selfStatusFileReadBuffer);
    if (selfStatusSetBufferResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_SET_BUFFER
        );
    }

    const File::SimpleResult selfStatusOpenResult = selfStatusFile.open(FileMode::READ_ONLY, false);
    if (selfStatusOpenResult.is_failure()) {
        return pican::Result<MemoryReader, MemoryReader::Error>::failure_by_copy(
            MemoryReader::Error::FAILED_TO_OPEN_FILE
        );
    }

    mem::Block lineBuffer = mem::Manager::get_block(256);

    return pican::Result<MemoryReader, MemoryReader::Error>::success_by_move(
        MemoryReader{std::move(memInfoFile), std::move(selfStatusFile), lineBuffer}
    );
}

// TODO @basshelal Wed 25-Feb-2026 : New design ideas!
//  read the entire file once, then filter through the contents depending on what we need from it, this needs a large
//  enough buffer so we need to plan ahead a little or just over-guess a lot


// TODO @basshelal Thu 26-Feb-2026 : New idea, use File without its internal buffers have, a huge local buffer to do
//  unbuffered reads into for EVERY file we will read, because we will read the files and fill the structs one by one
//  sequentially so we don't need to waste memory

}  // namespace pican::info
