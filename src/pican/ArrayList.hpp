#pragma once

#include <utility>

#include "pican/Array.hpp"
#include "pican/Contracts.hpp"
#include "pican/mem/Block.hpp"
#include "pican/mem/Manager.hpp"
#include "pican/mem/Utils.hpp"

namespace pican {
template<typename TP>
class ArrayList {
private:  // fields
    Array<TP> array_f;
    Count size_f;

public:  // constructors
    explicit ArrayList(const Array<TP>& array) : array_f{array} {
    }

public:  // copy-control
    ArrayList(const ArrayList& rhs) = default;

    ArrayList(ArrayList&& rhs) noexcept = default;

    ArrayList&
    operator=(const ArrayList& rhs) & = default;

    ArrayList&
    operator=(ArrayList&& rhs) & noexcept = default;

    ~ArrayList() = default;

public:  // member functions
    [[nodiscard]]
    TP*
    get_ptr(Index index) const& {
        return this->array_f.get_ptr(index);
    }

    [[nodiscard]]
    const TP&
    get(Index index) const& {
        return this->array_f.get(index);
    }

    [[nodiscard]]
    TP&
    get(Index index) & {
        return this->array_f.get(index);
    }

    void
    set(Index index, const TP& val) & {
        this->array_f.set(index, val);
    }

    void
    set_move(Index index, TP&& val) & {
        this->array_f.set_move(index, std::move(val));
    }

    void
    add_copy(const TP& val) {
        this->set(this->size_f++, val);
    }

    void
    add_move(TP&& val) {
        this->set_move(this->size_f++, std::move(val));
    }

public:  // operators
    TP&
    operator[](Index index) & {
        return this->array_f.operator[](index);
    }

    const TP&
    operator[](Index index) const& {
        return this->array_f.operator[](index);
    }

public:  // iterators
    BasicIterator<TP>
    begin() const& {
        return this->array_f.begin();
    }

    BasicIterator<TP>
    end() const& {
        const mem::Block& block = this->array_f.block();
        return BasicIterator<TP>{block.ptr_at_offset<TP>(this->size_f * sizeof(TP))};
    }

public:  // getters
    [[nodiscard]]
    inline const Array<TP>&
    array() const& {
        return this->array_f;
    }

    [[nodiscard]]
    inline Count
    capacity() const& {
        return this->array_f.length();
    }

    [[nodiscard]]
    inline Count
    size() const& {
        return this->size_f;
    }
};
}  // namespace pican
