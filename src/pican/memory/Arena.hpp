#pragma once

#include <cstdint>

#include "pican/Contracts.hpp"
#include "pican/Result.hpp"
#include "pican/Utils.hpp"
#include "pican/memory/Block.hpp"
#include "pican/memory/Utils.hpp"

namespace pican::memory {

class Manager;

// TODO @basshelal Wed 21-Jan-2026 : Documentation!
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
    get_block(SizeBytes size, Alignment alignment = SYSTEM_DEFAULT_ALIGNMENT) &;

    template<typename TP>
    [[nodiscard]]
    TP*
    get_ptr_uninitialized() & {
        const SizeBytes size = sizeof(TP);
        const Alignment alignment = alignof(TP);
        Block block = this->get_block(size, alignment);

        TP* ptr = block.address_to_ptr<TP>();
        if (ptr == pican::memory::ptr_to_address(nullptr)) {
            return nullptr;
        }
        CONTRACTS_ASSERT(block.size_bytes() >= size);
        CONTRACTS_ASSERT(pican::memory::address_is_aligned(ptr, alignment));

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

        return pican::memory::construct_at(ptr, std::forward<Args>(args)...);
    }

    void
    rewind_to(Offset offset) &;

    void
    reset() &;

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

}  // namespace pican::memory
