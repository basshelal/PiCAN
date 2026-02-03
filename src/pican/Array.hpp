#pragma once

#include "pican/Contracts.hpp"
#include "pican/memory/Block.hpp"
#include "pican/memory/Utils.hpp"

namespace pican {
template<typename TP>
class Array {
private:  // types
    using Block = pican::memory::Block;

public:  // types
    using Index = std::size_t;
    using Count = std::size_t;

private:  // constants
    static constexpr SizeBytes TP_SIZE = sizeof(TP);
    static constexpr Alignment TP_ALIGNMENT = alignof(TP);

private:  // fields
    pican::memory::Block block_f;
    Count itemsCount_f;

public:  // constructors
    explicit Array(const pican::memory::Block& block) : block_f{block}, itemsCount_f{block.size_bytes() / TP_SIZE} {
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
    get_at_ptr(Index index) const& {
        CONTRACTS_PRECONDITION(index < this->itemsCount_f);
        return this->block_f.ptr_at_offset<TP>(index * TP_SIZE);
    }

    [[nodiscard]]
    const TP&
    get_at(Index index) const& {
        CONTRACTS_PRECONDITION(index < this->itemsCount_f);
        return *this->get_at_ptr(index);
    }

    void
    set_at(Index index, const TP& val) & {
        CONTRACTS_PRECONDITION(index < this->itemsCount_f);
        *this->block_f.ptr_at_offset<TP>(index * TP_SIZE) = val;
    }

    void
    set_at_move(Index index, TP&& val) & {
        CONTRACTS_PRECONDITION(index < this->itemsCount_f);
        *this->block_f.ptr_at_offset<TP>(index * TP_SIZE) = std::move(val);
    }

public:  // getters
    [[nodiscard]]
    inline const pican::memory::Block&
    block() const& {
        return this->block_f;
    }

    [[nodiscard]]
    inline Count
    items_count() const& {
        return this->itemsCount_f;
    }

    [[nodiscard]]
    inline Index
    last_index() const& {
        return this->itemsCount_f - 1;
    }
};
}  // namespace pican
