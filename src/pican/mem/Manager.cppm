module;
#include <cstdint>

#include <sys/mman.h>

#include "pican/macros.hpp"
#include "pican/contracts.hpp"

export module pican.mem:Manager;

import pican.core;
// import pican.ds;
import :Arena;
import :Block;
import :Pool;
import :utils;

export namespace pican::mem {

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
    initialize(SizeBytes size = DEFAULT_SIZE) {
        if (This::instance_sf != nullptr) {
            pican::panic("Memory manager already initialized!");
        }
        const SizeBytes toAllocate = pican::clamp(MIN_SIZE, size, MAX_SIZE);

        void* memory = ::mmap(nullptr, toAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (memory == MAP_FAILED || memory == nullptr) {
            pican::panic("Could not initialize memory manager, mmap failed!");
        }

        const int result = ::mlockall(MCL_CURRENT | MCL_FUTURE);
        if (result != 0) {
            pican::panic("Could not initialize memory manager, mlockall failed!");
        }

        SANITY_CHECK(toAllocate > sizeof(Manager));

        This::instance_sf =
            new (static_cast<Manager*>(memory)) Manager{mem::ptr_to_address(memory), toAllocate, sizeof(Manager)};
        This::ensure_not_sealed();
    }

public:  // member functions
    [[nodiscard]]
    static Block
    get_block(SizeBytes size, Alignment alignment = pican::mem::SYSTEM_DEFAULT_ALIGNMENT) {
        This::ensure_not_sealed();
        Manager& instance = *This::instance_sf;

        const Address headAddress = This::head_address();

        const Alignment modulo = headAddress % alignment;
        SizeBytes paddingRequired = 0;
        if (modulo != 0) {
            paddingRequired = alignment - modulo;
        }

        const Offset newHeadOffset = instance.headOffset_f + paddingRequired + size;
        if (newHeadOffset > instance.sizeBytes_f) {
            pican::mem::panic_out_of_memory();
        }

        const Address address = headAddress + paddingRequired;
        CONTRACTS_ASSERT(pican::mem::address_is_aligned(address, alignment));
        instance.headOffset_f = newHeadOffset;

        return Block{address, size};
    }

    template<typename TP>
    [[nodiscard]]
    static inline Block
    get_block_for() {
        return This::get_block(sizeof(TP), alignof(TP));
    }

    [[nodiscard]]
    static Arena
    get_arena(SizeBytes size, Alignment alignment = pican::mem::SYSTEM_DEFAULT_ALIGNMENT) {
        This::ensure_not_sealed();
        Block block = This::get_block(size, alignment);

        return Arena{block};
    }

    template<typename TP>
    [[nodiscard]]
    static Pool<TP>
    get_pool(Count objectCount) {
        This::ensure_not_sealed();
        Block block = This::get_block(sizeof(TP) * objectCount, alignof(TP));
        return Pool<TP>{block};
    }

    static void
    seal() {
        This::ensure_initialized();
        This::instance_sf->isSealed_f = true;
    }

public:  // getters
    [[nodiscard]]
    static inline SizeBytes
    size_bytes() {
        This::ensure_initialized();
        Manager& instance = *This::instance_sf;
        return instance.sizeBytes_f;
    }

    [[nodiscard]]
    static inline SizeBytes
    used_bytes() {
        This::ensure_initialized();
        Manager& instance = *This::instance_sf;
        return instance.headOffset_f;
    }

    [[nodiscard]]
    static inline SizeBytes
    free_bytes() {
        This::ensure_initialized();
        Manager& instance = *This::instance_sf;
        return instance.sizeBytes_f - instance.headOffset_f;
    }

    [[nodiscard]]
    static inline bool
    is_sealed() {
        This::ensure_initialized();
        return This::instance_sf->isSealed_f;
    }

    [[nodiscard]]
    static bool
    is_initialized() {
        return This::instance_sf != nullptr;
    }

private:  // helper functions
    static void
    ensure_initialized() {
        if (This::instance_sf == nullptr) {
            pican::panic("MemoryManager not initialized!");
        }
    }

    static void
    ensure_not_sealed() {
        This::ensure_initialized();
        if (This::instance_sf->isSealed_f) {
            pican::panic("MemoryManager is sealed!");
        }
    }

    static Address
    head_address() {
        This::ensure_initialized();
        Manager& instance = *This::instance_sf;
        return instance.address_f + instance.headOffset_f;
    }
};
}  // namespace pican::mem
