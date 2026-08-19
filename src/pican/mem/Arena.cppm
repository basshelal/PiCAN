module;

#include <cstdint>

#include "pican/contracts.hpp"

export module pican.mem:Arena;

import pican.core;
import :Block;

export namespace pican::mem {

class Arena {
private:  // fields
    Address address_f;
    SizeBytes capacity_f;
    Offset headOffset_f;

private:  // constructors
    Arena(Address address, SizeBytes capacityBytes) : address_f{address}, capacity_f{capacityBytes}, headOffset_f{0} {
    }

    explicit Arena(const Block& block) : Arena{block.address(), block.size_bytes()} {
    }

public:  // lifetime
    Arena(const Arena& rhs) = delete;

    Arena(Arena&& rhs) noexcept = delete;

    Arena&
    operator=(const Arena& rhs) = delete;

    Arena&
    operator=(Arena&& rhs) noexcept = delete;

    ~Arena() = default;

public:  // member functions
    [[nodiscard]]
    Block
    get_block(SizeBytes size, Alignment alignment = SYSTEM_DEFAULT_ALIGNMENT) & {
        const Address headAddress = this->head_address();

        const Alignment modulo = headAddress % alignment;
        SizeBytes paddingRequired = 0;
        if (modulo != 0) {
            paddingRequired = alignment - modulo;
        }

        const Offset newHeadOffset = this->headOffset_f + paddingRequired + size;
        if (newHeadOffset > this->capacity_f) {
            return Block::NULL_BLOCK;
        }

        const Address address = this->head_address() + paddingRequired;
        CONTRACTS_ASSERT(pican::mem::address_is_aligned(address, alignment));
        this->headOffset_f = newHeadOffset;

        return Block{address, size};
    }

    template<typename TP>
    [[nodiscard]]
    TP*
    get_ptr_uninitialized() & {
        const SizeBytes size = sizeof(TP);
        const Alignment alignment = alignof(TP);
        Block block = this->get_block(size, alignment);

        TP* ptr = block.address_to_ptr<TP>();
        if (ptr == pican::mem::ptr_to_address(nullptr)) {
            return nullptr;
        }
        CONTRACTS_ASSERT(block.size_bytes() >= size);
        CONTRACTS_ASSERT(pican::mem::address_is_aligned(ptr, alignment));

        return ptr;
    }

    template<typename TP, typename... Args>
    [[nodiscard]]
    TP*
    get_ptr_initialized(Args&&... args) & {
        TP* ptr = this->get_ptr_uninitialized<TP>();
        if (ptr == nullptr) {
            return nullptr;
        }

        return new (ptr) TP{std::forward<Args>(args)...};
    }

    void
    rewind_to(Offset offset) & {
        if (offset > this->headOffset_f || offset > this->capacity_f) {
            return;
        }
        this->headOffset_f = offset;
    }

    void
    reset() & {
        this->rewind_to(0);
    }

public:  // getters
    [[nodiscard]]
    inline SizeBytes
    capacity() const& {
        return this->capacity_f;
    }

    [[nodiscard]]
    inline Offset
    head_offset() const& {
        return this->headOffset_f;
    }

    [[nodiscard]]
    inline SizeBytes
    used_bytes() const& {
        return this->headOffset_f;
    }

    [[nodiscard]]
    inline SizeBytes
    free_bytes() const& {
        return this->capacity_f - this->headOffset_f;
    }

    [[nodiscard]]
    inline bool
    is_full() const& {
        return this->headOffset_f >= this->capacity_f;
    }

    [[nodiscard]]
    inline Address
    head_address() const& {
        return this->address_f + this->headOffset_f;
    }

    [[nodiscard]]
    inline Address
    end_address() const& {
        return this->address_f + this->capacity_f;
    }

public:  // friends
    friend class Manager;
};

}  // namespace pican::mem
