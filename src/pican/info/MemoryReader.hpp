#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

#include "pican/File.hpp"
#include "pican/Result.hpp"
#include "pican/Utils.hpp"

namespace pican::info {

class MemoryReader {
private:  // types
    struct Info {
        SizeBytes totalMemory;
        SizeBytes availableMemory;
        SizeBytes processUsedMemory;
    };

public:  // types
    enum class Error : std::uint8_t {
        FAILED_TO_SET_BUFFER,
        FAILED_TO_OPEN_FILE,
        FILE_READ_ERROR,
    };

private:  // fields
    pican::File memInfoFile_f;
    pican::File selfStatusFile_f;
    mem::Block lineBuffer_f;
    Info info_f;

private:  // constructors
    MemoryReader(pican::File&& memInfoFile, pican::File&& selfStatusFile, const mem::Block& lineBuffer);

public:  // lifetime
    MemoryReader(const MemoryReader& rhs) = delete;

    MemoryReader(MemoryReader&& rhs) noexcept = default;

    MemoryReader&
    operator=(const MemoryReader& rhs) & = delete;

    MemoryReader&
    operator=(MemoryReader&& rhs) & noexcept = default;

    ~MemoryReader() = default;

public:  // member functions
    SimpleResult<Error>
    update_info() &;

    [[nodiscard]]
    SizeBytes
    get_total_system_memory() const&;

    [[nodiscard]]
    SizeBytes
    get_free_system_memory() const&;

    [[nodiscard]]
    SizeBytes
    get_used_system_memory() const&;

    [[nodiscard]]
    SizeBytes
    get_process_used_memory() const&;

private:  // member functions
    SimpleResult<Error>
    read_meminfo_file() &;

public:  // static functions
    [[nodiscard]]
    static pican::Result<MemoryReader, MemoryReader::Error>
    create();
};

}  // namespace pican::info
