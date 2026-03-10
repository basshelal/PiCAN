#include "pican/FileBuffer.hpp"

#include "pican/Contracts.hpp"

namespace pican {
FileBuffer::FileBuffer(const mem::Block& block) : block_f{block}, writeIndex_f{0}, readIndex_f{0} {
    CONTRACTS_PRECONDITION(!block.is_null());
}

SizeBytes
FileBuffer::write_from(void* source, SizeBytes size) & {
    CONTRACTS_PRECONDITION(source != nullptr);

    SizeBytes bytesToWrite = std::min(this->writable_bytes(), size);

    char* destPtr = this->block_f.address_to_ptr<char>() + this->writeIndex_f;
    std::memcpy(destPtr, source, bytesToWrite);

    this->writeIndex_f += bytesToWrite;

    CONTRACTS_ASSERT(this->writeIndex_f <= this->capacity_bytes());
    CONTRACTS_ASSERT(this->writeIndex_f >= this->readIndex_f);
    return bytesToWrite;
}

SizeBytes
FileBuffer::read_into(void* destination, SizeBytes size) const& {
    CONTRACTS_PRECONDITION(destination != nullptr);

    SizeBytes bytesToRead = std::min(this->readable_bytes(), size);
    if (bytesToRead == 0) {
        return bytesToRead;
    }

    char* srcPtr = this->block_f.address_to_ptr<char>() + this->readIndex_f;
    std::memcpy(destination, srcPtr, bytesToRead);

    this->readIndex_f += bytesToRead;

    CONTRACTS_ASSERT(this->readIndex_f <= this->capacity_bytes());
    CONTRACTS_ASSERT(this->readIndex_f <= this->writeIndex_f);
    return bytesToRead;
}

void
FileBuffer::clear() & {
    this->readIndex_f = 0;
    this->writeIndex_f = 0;
}

const mem::Block&
FileBuffer::block() const& {
    return this->block_f;
}

Index
FileBuffer::read_index() const& {
    return this->readIndex_f;
}

Index
FileBuffer::write_index() const& {
    return this->writeIndex_f;
}

SizeBytes
FileBuffer::readable_bytes() const& {
    return this->writeIndex_f - this->readIndex_f;
}

SizeBytes
FileBuffer::writable_bytes() const& {
    return this->capacity_bytes() - this->writeIndex_f;
}

SizeBytes
FileBuffer::capacity_bytes() const& {
    return this->block_f.size_bytes();
}

Index
FileBuffer::increment_write_index_by(Index incrementBy) & {
    const Index actual = pican::clamp<Index>(0, incrementBy, static_cast<Index>(this->writable_bytes()));
    this->writeIndex_f += actual;
    CONTRACTS_ASSERT(this->writeIndex_f <= this->capacity_bytes());
    return this->writeIndex_f;
}

Index
FileBuffer::increment_read_index_by(Index incrementBy) & {
    const Index actual = pican::clamp<Index>(0, incrementBy, static_cast<Index>(this->readable_bytes()));
    this->readIndex_f += actual;
    CONTRACTS_ASSERT(this->readIndex_f <= this->writeIndex_f);
    return this->readIndex_f;
}

}  // namespace pican
