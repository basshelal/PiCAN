module;

#include <cstdint>
#include <cstring>
#include <algorithm>

export module pican.fs:FileBuffer;

import pican.contracts;
import pican.core;
import pican.mem;

export namespace pican::fs {
class FileBuffer {
public:   // types
private:  // fields
    mem::Block block_f;
    mutable Index writeIndex_f;
    mutable Index readIndex_f;

public:  // constructors
    explicit FileBuffer(const mem::Block& block) : block_f{block}, writeIndex_f{0}, readIndex_f{0} {
        pican::contracts::precondition(!block.is_null());
    }

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
    write_from(void* source, SizeBytes size) & {
        pican::contracts::precondition(source != nullptr);

        SizeBytes bytesToWrite = std::min(this->writable_bytes(), size);

        char* destPtr = this->block_f.address_to_ptr<char>() + this->writeIndex_f;
        std::memcpy(destPtr, source, bytesToWrite);

        this->writeIndex_f += bytesToWrite;

        pican::contracts::assertion(this->writeIndex_f <= this->capacity_bytes());
        pican::contracts::assertion(this->writeIndex_f >= this->readIndex_f);
        return bytesToWrite;
    }

    SizeBytes
    read_into(void* destination, SizeBytes size) const& {
        pican::contracts::precondition(destination != nullptr);

        SizeBytes bytesToRead = std::min(this->readable_bytes(), size);
        if (bytesToRead == 0) {
            return bytesToRead;
        }

        char* srcPtr = this->block_f.address_to_ptr<char>() + this->readIndex_f;
        std::memcpy(destination, srcPtr, bytesToRead);

        this->readIndex_f += bytesToRead;

        pican::contracts::assertion(this->readIndex_f <= this->capacity_bytes());
        pican::contracts::assertion(this->readIndex_f <= this->writeIndex_f);
        return bytesToRead;
    }

    void
    clear() & {
        this->readIndex_f = 0;
        this->writeIndex_f = 0;
    }

    [[nodiscard]]
    const mem::Block&
    block() const& {
        return this->block_f;
    }

    [[nodiscard]]
    Index
    read_index() const& {
        return this->readIndex_f;
    }

    [[nodiscard]]
    Index
    write_index() const& {
        return this->writeIndex_f;
    }

    [[nodiscard]]
    SizeBytes
    readable_bytes() const& {
        return this->writeIndex_f - this->readIndex_f;
    }

    [[nodiscard]]
    SizeBytes
    writable_bytes() const& {
        return this->capacity_bytes() - this->writeIndex_f;
    }

    [[nodiscard]]
    SizeBytes
    capacity_bytes() const& {
        return this->block_f.size_bytes();
    }

    Index
    increment_write_index_by(Index incrementBy) & {
        const Index actual = pican::clamp<Index>(0, incrementBy, static_cast<Index>(this->writable_bytes()));
        this->writeIndex_f += actual;
        pican::contracts::assertion(this->writeIndex_f <= this->capacity_bytes());
        return this->writeIndex_f;
    }

    Index
    increment_read_index_by(Index incrementBy) & {
        const Index actual = pican::clamp<Index>(0, incrementBy, static_cast<Index>(this->readable_bytes()));
        this->readIndex_f += actual;
        pican::contracts::assertion(this->readIndex_f <= this->writeIndex_f);
        return this->readIndex_f;
    }

public:  // friends
    friend class File;
};
}  // namespace pican
