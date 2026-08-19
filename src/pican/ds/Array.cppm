module;

#include "pican/contracts.hpp"
#include "pican/macros.hpp"

export module pican.ds:Array;

import pican.core;
import pican.mem;
import :BasicIterator;

export namespace pican::ds {
template<typename TP>
class Array {
private:  // constants
    static constexpr SizeBytes TP_SIZE = sizeof(TP);

private:  // fields
    mem::Block block_f;
    Count length_f;

private:  // constructors
    Array(const mem::Block& block) : block_f{block}, length_f{block.size_bytes() / TP_SIZE} {
    }

public:  // static factories
    static Array<TP>
    initialize_by_copy(const mem::Block& block, const TP& initialValue) {
        CONTRACTS_PRECONDITION(block.size_bytes() >= TP_SIZE);  // needs at least 1 element
        Array<TP> array{block};
        std::uninitialized_fill_n(array.block_f.address_to_ptr<TP>(), array.length_f, initialValue);
        return array;
    }

    template<typename... Args_TP>
    static Array<TP>
    initialize_emplace(const mem::Block& block, Args_TP&&... args) {
        CONTRACTS_PRECONDITION(block.size_bytes() >= TP_SIZE);  // needs at least 1 element
        Array<TP> array{block};
        for (BasicIterator<TP> iter = array.begin(); iter != array.end(); ++iter) {
            TP* ptr = iter.get();
            new (ptr) TP{std::forward<Args_TP>(args)...};
        }
        return array;
    }

public:  // lifetime
    Array(const Array& rhs) = default;

    Array(Array&& rhs) noexcept = default;

    Array&
    operator=(const Array& rhs) = default;

    Array&
    operator=(Array&& rhs) noexcept = default;

    ~Array() {
        std::destroy_n(this->block_f.address_to_ptr<TP>(), this->length_f);
    }

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
    set_copy(Index index, const TP& val) & {
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
