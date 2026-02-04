#pragma once

#include <cstdint>

#include "pican/RingBuffer.hpp"
#include "pican/mem/Arena.hpp"
#include "pican/mem/Block.hpp"
#include "pican/mem/Pool.hpp"
#include "pican/mem/Utils.hpp"

namespace pican::mem {

class Manager {
private:  // types
    using This = Manager;

public:                                                             // constants
    static constexpr SizeBytes MIN_SIZE = 512 * 1'024;              // 512 KiB
    static constexpr SizeBytes MAX_SIZE = 420 * 1'024 * 1'024;      // 420 MiB
    static constexpr SizeBytes DEFAULT_SIZE = 400 * 1'024 * 1'024;  // 400 MiB
private:                                                            // static fields
    static Manager* instance_sf;

private:  // fields
    Address address_f;
    SizeBytes sizeBytes_f;
    Offset headOffset_f;
    bool isSealed_f;

private:  // constructor
    Manager(Address address, SizeBytes size, Offset headOffset) :
        address_f{address}, sizeBytes_f{size}, headOffset_f{headOffset}, isSealed_f{false} {
    }

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

public:  // member functions
    [[nodiscard]]
    static Block
    get_block(SizeBytes size, Alignment alignment = pican::mem::SYSTEM_DEFAULT_ALIGNMENT);

    [[nodiscard]]
    static Arena
    get_arena(SizeBytes size, Alignment alignment = pican::mem::SYSTEM_DEFAULT_ALIGNMENT);

    template<typename TP>
    [[nodiscard]]
    static Pool<TP>
    get_pool(Count objectCount) {
        This::ensure_not_sealed();
        Block block = This::get_block(sizeof(TP) * objectCount, alignof(TP));
        return Pool<TP>{block};
    }

    template<typename TP>
    [[nodiscard]]
    static Array<TP>
    get_array(Count objectCount) {
        This::ensure_not_sealed();
        Block block = This::get_block(sizeof(TP) * objectCount, alignof(TP));
        return Array<TP>{block};
    }

    static void
    seal();

public:  // getters
    [[nodiscard]]
    static inline SizeBytes
    size_bytes();

    [[nodiscard]]
    static inline SizeBytes
    used_bytes();

    [[nodiscard]]
    static inline SizeBytes
    free_bytes();

    [[nodiscard]]
    static inline bool
    is_sealed();

    [[nodiscard]]
    static bool
    is_initialized();

private:  // helper functions
    static void
    ensure_initialized();

    static void
    ensure_not_sealed();

    static Address
    head_address();
};
}  // namespace pican::mem
