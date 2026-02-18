#pragma once

#include <cassert>
#include <cstdint>
#include <optional>

#include "pican/Array.hpp"
#include "pican/Utils.hpp"

namespace pican {
template<typename Key_TP, typename Value_TP>
class Map {
private:  // types
    enum class Status : std::uint8_t {
        EMPTY,
        OCCUPIED,
    };

    struct Element {
        Key_TP key;
        Value_TP value;
        Status status;
    };

private:  // fields
    Array<Element> elements_f;
    Count elementsCount_f;

public:  // constructors
    explicit Map(Array<Element> elements) : elements_f{elements}, elementsCount_f{0} {
    }

public:  // lifetime
    Map(const Map& rhs) = delete;

    Map(Map&& rhs) noexcept = delete;

    Map&
    operator=(const Map& rhs) & = delete;

    Map&
    operator=(Map&& rhs) & noexcept = delete;

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
        return this->elementsCount_f;
    }

public:  // member functions
    void
    put(const Key_TP& key, const Value_TP& value) & {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        assert(element != nullptr);

        if (element->status == Status::EMPTY) {
            this->elements_f[index] = Element{key, value, Status::OCCUPIED};
            return;
        }
        assert(element->status == Status::OCCUPIED);
        // already exists, what do we do??
    }

    [[nodiscard]]
    std::optional<const Value_TP&>
    get(const Key_TP& key) const& {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        assert(element != nullptr);
        if (element->status == Status::EMPTY) {
            return std::optional<const Value_TP&>{};
        }
        return std::optional<const Value_TP&>{element->value};
    }

    [[nodiscard]]
    bool
    has_value(const Key_TP& key) const& {
        Index index = this->hashed_index(key);
        Element* element = this->elements_f.get_ptr(index);
        assert(element != nullptr);
        return element->status != Status::EMPTY;
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
}  // namespace pican
