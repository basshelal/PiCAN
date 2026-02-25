#include "pican/FileBuffer.hpp"

#include "pican/Contracts.hpp"

namespace pican {
FileBuffer::FileBuffer(const mem::Block& block, FileBuffer::Type type) : block_f{block}, offset_f{0}, type_f{type} {
    CONTRACTS_PRECONDITION(!block.is_null());
}

SizeBytes
FileBuffer::write_from(void* source, SizeBytes size) & {
    CONTRACTS_PRECONDITION(source != nullptr);
    CONTRACTS_PRECONDITION(this->type_f == FileBuffer::Type::WRITE);
    CONTRACTS_POSTCONDITION(this->offset_f <= this->block_f.size_bytes());

    SizeBytes bytesToWrite = std::min(this->remaining_bytes(), size);

    std::memcpy(this->block_f.address_to_ptr<void>(), source, bytesToWrite);

    this->offset_f += bytesToWrite;
    return bytesToWrite;
}

SizeBytes
FileBuffer::read_into(void* destination, SizeBytes size) const& {
    CONTRACTS_PRECONDITION(destination != nullptr);
    CONTRACTS_PRECONDITION(this->type_f == FileBuffer::Type::READ);
    CONTRACTS_POSTCONDITION(this->offset_f <= this->block_f.size_bytes());

    SizeBytes bytesToRead = std::min(this->remaining_bytes(), size);

    std::memcpy(destination, this->block_f.address_to_ptr<void>(), bytesToRead);

    this->offset_f += bytesToRead;
    return bytesToRead;
}

Offset
FileBuffer::set_offset(Offset offset) & {
    CONTRACTS_POSTCONDITION(this->offset_f <= this->block_f.size_bytes());

    this->offset_f = pican::clamp(static_cast<SizeBytes>(0), static_cast<SizeBytes>(offset), this->capacity_bytes());
    return this->offset_f;
}

const mem::Block&
FileBuffer::block() const& {
    return this->block_f;
}

Offset
FileBuffer::offset() const& {
    return this->offset_f;
}

FileBuffer::Type
FileBuffer::type() const& {
    return this->type_f;
}

SizeBytes
FileBuffer::remaining_bytes() const& {
    return this->block_f.size_bytes() - this->offset_f;
}

SizeBytes
FileBuffer::capacity_bytes() const& {
    return this->block_f.size_bytes();
}

bool
FileBuffer::is_full() const& {
    return this->remaining_bytes() == 0;
}
}  // namespace pican
