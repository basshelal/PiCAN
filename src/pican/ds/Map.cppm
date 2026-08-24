module;

#include <cassert>
#include <cstdint>
#include <optional>

#include "pican/contracts.hpp"
#include "pican/macros.hpp"

export module pican.ds:Map;

import :ArrayList;
import pican.mem;

namespace pican::ds {
enum class ElementStatus : std::uint8_t {
    EMPTY,
    OCCUPIED,
};
}  // namespace pican::ds

export namespace pican::ds {
template<typename Key_TP, typename Value_TP>
class Map {
public:  // types
    struct Entry {
        Key_TP key;
        Value_TP value;
    };

private:  // types
    struct Element {
        ElementStatus status = ElementStatus::EMPTY;
        Key_TP key;
        Value_TP value;
    };

public:  // static constants
    static constexpr SizeBytes ELEMENT_SIZE = sizeof(Element);

private:  // fields
    ArrayList<Element> elements_f;

public:  // constructors
    explicit Map(mem::Block block) : elements_f{block} {
    }

public:  // named constructors
    [[nodiscard]]
    static Map
    create(mem::Block block, const Key_TP& initialKey, const Value_TP& initialValue) {
        const Count elementCount = block.size_bytes() % ELEMENT_SIZE;
        const Element defaultElement{ElementStatus::EMPTY, initialKey, initialValue};
        return Map{ArrayList<Element>::initialize_by_copy(defaultElement)};
    }

public:  // lifetime
    Map(const Map& rhs) = default;

    Map(Map&& rhs) noexcept = default;

    Map&
    operator=(const Map& rhs) & = default;

    Map&
    operator=(Map&& rhs) & noexcept = default;

    ~Map() = default;

public:  // getters
    [[nodiscard]]
    inline Count
    capacity() const& {
        return this->elements_f.length();
    }

    [[nodiscard]]
    inline Count
    size() const& {
        return this->elements_f.size();
    }

public:  // member functions
    void
    put_copy(const Key_TP& key, const Value_TP& value) & {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        CONTRACTS_ASSERT(element != nullptr);

        // empty slot
        if (element->status == ElementStatus::EMPTY) {
            element->key = key;
            element->value = value;
            element->status = ElementStatus::OCCUPIED;
            return;
        }

        CONTRACTS_ASSERT(element->status == ElementStatus::OCCUPIED);
        // already exists but key is same, update
        if (element->key == key) {
            element->value = value;
            return;
        }
        // already exists but key is different, hashing collision, find next free slot
        TODO_NOT_IMPLEMENTED();
    }

    [[nodiscard]]
    std::optional<Value_TP>
    get(const Key_TP& key) const& {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        assert(element != nullptr);
        if (element->status == ElementStatus::EMPTY) {
            return std::optional<Value_TP>{};
        }
        return std::optional<Value_TP>{element->value};
    }

    [[nodiscard]]
    bool
    has_value(const Key_TP& key) const& {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        assert(element != nullptr);
        return element->status != ElementStatus::EMPTY;
    }

private:  // member functions
    [[nodiscard]]
    Index
    hashed_index(const Key_TP& key) const& {
        Index hash = pican::hash(key);
        Index index = hash % this->capacity();
        assert(index >= 0 && index < this->capacity());
        return index;
    }
};
}  // namespace pican::ds
