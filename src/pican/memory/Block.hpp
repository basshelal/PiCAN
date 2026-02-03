#pragma once

#include <cstdint>

#include "../../../build/debug/_deps/googletest-src/googletest/include/gtest/gtest_prod.h"
#include "pican/memory/Utils.hpp"

namespace pican::memory {

class Manager;
class Arena;

class Block {
public:  // constants
    static const Block NULL_BLOCK;

private:  // fields
    Address address_f;
    SizeBytes sizeBytes_f;

public:  // constructors
    Block(Address address, SizeBytes sizeBytes) : address_f{address}, sizeBytes_f{sizeBytes} {
    }

public:  // copy-control
    Block(const Block& rhs) = default;

    Block(Block&& rhs) noexcept = default;

    Block&
    operator=(const Block& rhs) & = default;

    Block&
    operator=(Block&& rhs) & noexcept = default;

    ~Block() = default;

public:  // member functions
    [[nodiscard]]
    inline bool
    is_aligned(Alignment alignment) const& {
        return pican::memory::address_is_aligned(this->address_f, alignment);
    }

    [[nodiscard]]
    inline Address
    address_at_offset(Offset offset) const& {
        return this->address_f + offset;
    }

    template<typename TP>
    [[nodiscard]]
    inline TP*
    ptr_at_offset(Offset offset) const& {
        return pican::memory::address_to_ptr<TP>(this->address_f + offset);
    }

    [[nodiscard]]
    inline bool
    contains_address(Address address) const& {
        return address >= this->address_f && address < this->end_address();
    }

public:  // getters
    [[nodiscard]]
    inline Address
    address() const& {
        return this->address_f;
    }

    [[nodiscard]]
    inline SizeBytes
    size_bytes() const& {
        return this->sizeBytes_f;
    }

    [[nodiscard]]
    inline bool
    is_null() const& {
        return this->address_f == NULL_ADDRESS;
    }

    [[nodiscard]]
    inline bool
    has_known_size() const& {
        return this->sizeBytes_f != UNKNOWN_SIZE;
    }

    template<typename TP = void>
    [[nodiscard]]
    inline TP*
    address_to_ptr() const& {
        return pican::memory::address_to_ptr<TP>(this->address_f);
    }

    [[nodiscard]]
    inline Address
    end_address() const& {
        return this->address_f + this->sizeBytes_f;
    }

public:  // friends
    friend class Manager;
    friend class Arena;
};
}  // namespace pican::memory
