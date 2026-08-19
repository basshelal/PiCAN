module;

#include <utility>

#include "pican/contracts.hpp"

export module pican.ds:ArrayList;

import pican.core;
import pican.mem;
import :Array;
import :BasicIterator;

export namespace pican::ds {
template<typename TP>
class ArrayList {
private:  // fields
    Array<TP> array_f;
    Count size_f;

public:  // constructors
    explicit ArrayList(const Array<TP>& array) : array_f{array} {
    }

public:  // named constructors
    [[nodiscard]]
    static ArrayList<TP>
    initialize_by_copy(const mem::Block& block, const TP& initialValue) {
        return ArrayList<TP>{Array<TP>::initialize_by_copy(block, initialValue)};
    }

    template<typename... Args_TP>
    static ArrayList<TP>
    initialize_emplace(const mem::Block& block, Args_TP&&... args) {
        return ArrayList<TP>{Array<TP>::initialize_emplace(block, std::forward<Args_TP>(args)...)};
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
    set_copy(Index index, const TP& val) & {
        this->array_f.set_copy(index, val);
    }

    void
    set_move(Index index, TP&& val) & {
        this->array_f.set_move(index, std::move(val));
    }

    void
    add_copy(const TP& val) {
        this->set_copy(this->size_f++, val);
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
}  // namespace pican::ds
