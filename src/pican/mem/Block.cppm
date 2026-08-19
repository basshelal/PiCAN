module;

export module pican.mem:Block;

import pican.core;
import :utils;

export namespace pican::mem {

/**
 * A block of memory, essentially an Address and its SizeBytes which may be unknown
 */
class Block {
private:  // Data Members
    Address address_m;
    SizeBytes sizeBytes_m;

private:  // Constructors
    constexpr Block(Address address, SizeBytes sizeBytes) : address_m{address}, sizeBytes_m{sizeBytes} {
    }

public:  // Named Constructors
    [[nodiscard]]
    static Block
    from_address(Address address, SizeBytes sizeBytes) {
        return Block{address, sizeBytes};
    }

    template<typename TP>
    [[nodiscard]]
    static Block
    from_ptr(TP* ptr, SizeBytes sizeBytes) {
        return Block{ptr_to_address<TP>(ptr), sizeBytes};
    }

public:  // Special Member Functions
    Block() = delete;

    Block(const Block& rhs) = default;

    Block(Block&& rhs) noexcept = default;

    Block&
    operator=(const Block& rhs) & = default;

    Block&
    operator=(Block&& rhs) & noexcept = default;

    ~Block() = default;

public:  // Member Functions
    [[nodiscard]]
    bool
    is_aligned(Alignment alignment) const& {
        return pican::mem::address_is_aligned(this->address_m, alignment);
    }

    [[nodiscard]]
    Address
    address_at_offset(Offset offset) const& {
        return this->address_m + offset;
    }

    template<typename TP>
    [[nodiscard]]
    TP*
    ptr_at_offset(Offset offset) const& {
        return pican::mem::address_to_ptr<TP>(this->address_m + offset);
    }

    [[nodiscard]]
    bool
    contains_address(Address address) const& {
        return address >= this->address_m && address < this->end_address();
    }

public:  // Getters
    [[nodiscard]]
    Address
    address() const& {
        return this->address_m;
    }

    [[nodiscard]]
    SizeBytes
    size_bytes() const& {
        return this->sizeBytes_m;
    }

    [[nodiscard]]
    bool
    is_null() const& {
        return this->address_m == NULL_ADDRESS;
    }

    [[nodiscard]]
    bool
    has_known_size() const& {
        return this->sizeBytes_m != UNKNOWN_SIZE;
    }

    template<typename TP>
    [[nodiscard]]
    TP*
    address_to_ptr() const& {
        return pican::mem::address_to_ptr<TP>(this->address_m);
    }

    [[nodiscard]]
    Address
    end_address() const& {
        return this->address_m + this->sizeBytes_m;
    }

public:  // Operator Functions
    [[nodiscard]]
    bool
    operator==(const Block& other) const = default;

    [[nodiscard]]
    bool
    operator!=(const Block& other) const = default;

public:  // Constants
    static const Block NULL_BLOCK;

public:  // Friends
    friend class Manager;
    friend class Arena;
};

inline constexpr Block Block::NULL_BLOCK = Block{NULL_ADDRESS, UNKNOWN_SIZE};
}  // namespace pican::mem
