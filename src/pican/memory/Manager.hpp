#pragma once

#include <cstdint>

#include "pican/RingBuffer.hpp"
#include "pican/memory/Arena.hpp"
#include "pican/memory/Block.hpp"
#include "pican/memory/Pool.hpp"
#include "pican/memory/Utils.hpp"

namespace pican::memory {

class Manager {
public:                                                             // constants
    static constexpr SizeBytes MIN_SIZE = 512 * 1'024;              // 512 KiB
    static constexpr SizeBytes MAX_SIZE = 420 * 1'024 * 1'024;      // 420 MiB
    static constexpr SizeBytes DEFAULT_SIZE = 400 * 1'024 * 1'024;  // 400 MiB
private:                                                            // static fields
    static Manager instance_sf;
    static bool initialized_sf;

private:  // fields
    Address address_f;
    SizeBytes sizeBytes_f;
    Offset headOffset_f;
    bool isSealed_f;

private:  // constructor
    Manager() = default;

public:  // lifetime
    Manager(const Manager& rhs) = delete;

    Manager(Manager&& rhs) noexcept = delete;

    Manager&
    operator=(const Manager& rhs) = delete;

    Manager&
    operator=(Manager&& rhs) noexcept = delete;

    ~Manager() = default;

public:  // static functions
    static void
    initialize(SizeBytes size = DEFAULT_SIZE);

    [[nodiscard]]
    static Manager&
    get();

public:  // member functions
    [[nodiscard]]
    Block
    get_block(SizeBytes size, Alignment alignment = pican::memory::SYSTEM_DEFAULT_ALIGNMENT) &;

    [[nodiscard]]
    Arena
    get_arena(SizeBytes size, Alignment alignment = pican::memory::SYSTEM_DEFAULT_ALIGNMENT) &;

    template<typename TP>
    [[nodiscard]]
    Pool<TP>
    get_pool(Count objectCount) & {
        this->ensure_not_sealed();
        Block block = this->get_block(sizeof(TP) * objectCount, alignof(TP));
        return Pool<TP>{block};
    }

    void
    seal() &;

public:  // getters
    [[nodiscard]]
    inline SizeBytes
    size_bytes() const& {
        return this->sizeBytes_f;
    }

    [[nodiscard]]
    inline SizeBytes
    used_bytes() const& {
        return this->headOffset_f;
    }

    [[nodiscard]]
    inline SizeBytes
    free_bytes() const& {
        return this->sizeBytes_f - this->headOffset_f;
    }

    [[nodiscard]]
    inline bool
    is_sealed() const& {
        return this->isSealed_f;
    }

private:  // helper functions
    void
    ensure_not_sealed() const&;

    Address
    head_address() const&;
};
}  // namespace pican::memory
