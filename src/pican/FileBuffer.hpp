#pragma once

#include <cstdint>

#include "pican/Types.hpp"
#include "pican/mem/Block.hpp"

namespace pican {
class FileBuffer {
public:   // types
private:  // fields
    mem::Block block_f;
    mutable Index writeIndex_f;
    mutable Index readIndex_f;

public:  // constructors
    explicit FileBuffer(const mem::Block& block);

public:  // lifetime
    FileBuffer(const FileBuffer& rhs) = delete;

    FileBuffer(FileBuffer&& rhs) noexcept = default;

    FileBuffer&
    operator=(const FileBuffer& rhs) & = delete;

    FileBuffer&
    operator=(FileBuffer&& rhs) & noexcept = default;

    ~FileBuffer() = default;

public:  // member functions
    SizeBytes
    write_from(void* source, SizeBytes size) &;

    SizeBytes
    read_into(void* destination, SizeBytes size) const&;

    void
    clear() &;

    [[nodiscard]]
    const mem::Block&
    block() const&;

    [[nodiscard]]
    Index
    read_index() const&;

    [[nodiscard]]
    Index
    write_index() const&;

    [[nodiscard]]
    SizeBytes
    readable_bytes() const&;

    [[nodiscard]]
    SizeBytes
    writable_bytes() const&;

    [[nodiscard]]
    SizeBytes
    capacity_bytes() const&;

    Index
    increment_write_index_by(Index incrementBy) &;

    Index
    increment_read_index_by(Index incrementBy) &;

public:  // friends
    friend class File;
};
}  // namespace pican
