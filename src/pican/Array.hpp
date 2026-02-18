#pragma once

#include "pican/BasicIterator.hpp"
#include "pican/Contracts.hpp"
#include "pican/Types.hpp"
#include "pican/mem/Block.hpp"
#include "pican/mem/Utils.hpp"

namespace pican {
template<typename TP>
class Array {
private:  // constants
    static constexpr SizeBytes TP_SIZE = sizeof(TP);

private:  // fields
    mem::Block block_f;
    Count length_f;

public:  // constructors
    explicit Array(const mem::Block& block) : block_f{block}, length_f{block.size_bytes() / TP_SIZE} {
    }

    explicit Array(TP* ptr, SizeBytes sizeBytes) : Array{mem::Block{mem::ptr_to_address(ptr), sizeBytes}} {
    }

public:  // copy-control
    Array(const Array& rhs) = default;

    Array(Array&& rhs) noexcept = default;

    Array&
    operator=(const Array& rhs) = default;

    Array&
    operator=(Array&& rhs) noexcept = default;

    ~Array() = default;

public:  // member functions
    [[nodiscard]]
    TP*
    get_ptr(Index index) const& {
        CONTRACTS_PRECONDITION(index < this->length_f);
        return this->block_f.ptr_at_offset<TP>(index * TP_SIZE);
    }

    [[nodiscard]]
    const TP&
    get(Index index) const& {
        return *this->get_ptr(index);
    }

    [[nodiscard]]
    TP&
    get(Index index) & {
        return *this->get_ptr(index);
    }

    void
    set(Index index, const TP& val) & {
        *this->get_ptr(index) = val;
    }

    void
    set_move(Index index, TP&& val) & {
        *this->get_ptr(index) = std::move(val);
    }

public:  // operators
    TP&
    operator[](Index index) & {
        return *this->get_ptr(index);
    }

    const TP&
    operator[](Index index) const& {
        return *this->get_ptr(index);
    }

public:  // iterators
    BasicIterator<TP>
    begin() const& {
        return BasicIterator<TP>{this->get_ptr(0)};
    }

    BasicIterator<TP>
    end() const& {
        return BasicIterator<TP>{this->block_f.ptr_at_offset<TP>(this->length_f * TP_SIZE)};
    }

public:  // getters
    [[nodiscard]]
    const pican::mem::Block&
    block() const& {
        return this->block_f;
    }

    [[nodiscard]]
    Address
    address() const& {
        return this->block_f.address();
    }

    [[nodiscard]]
    TP*
    ptr() const& {
        return this->block_f.address_to_ptr<TP>();
    }

    [[nodiscard]]
    Count
    length() const& {
        return this->length_f;
    }

    [[nodiscard]]
    Index
    last_index() const& {
        return this->length_f - 1;
    }
};
}  // namespace pican
