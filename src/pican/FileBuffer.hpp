#pragma once

#include <cstdint>

#include "pican/Types.hpp"
#include "pican/mem/Block.hpp"

namespace pican {
class FileBuffer {
public:  // types
    enum class Type : std::uint8_t {
        READ,
        WRITE,
    };

private:  // fields
    mem::Block block_f;
    mutable Offset offset_f;
    FileBuffer::Type type_f;

public:  // constructors
    explicit FileBuffer(const mem::Block& block, FileBuffer::Type type);

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

    Offset
    set_offset(Offset offset) &;

    [[nodiscard]]
    const mem::Block&
    block() const&;

    [[nodiscard]]
    Offset
    offset() const&;

    [[nodiscard]]
    FileBuffer::Type
    type() const&;

    [[nodiscard]]
    SizeBytes
    remaining_bytes() const&;

    [[nodiscard]]
    SizeBytes
    capacity_bytes() const&;

    [[nodiscard]]
    bool
    is_full() const&;

public:  // friends
    friend class File;
};
}  // namespace pican
