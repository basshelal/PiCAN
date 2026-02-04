#include "pican/mem/Manager.hpp"

#include <sys/mman.h>

#include "pican/Utils.hpp"

namespace pican::mem {

Manager* Manager::instance_sf = nullptr;

/* static */
void
Manager::initialize(SizeBytes size) {
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

/* static */
Block
Manager::get_block(SizeBytes size, Alignment alignment) {
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

/* static */
Arena
Manager::get_arena(SizeBytes size, Alignment alignment) {
    This::ensure_not_sealed();
    Block block = This::get_block(size, alignment);

    return Arena{block};
}

/* static */
void
Manager::seal() {
    This::ensure_initialized();
    This::instance_sf->isSealed_f = true;
}

/* static */
SizeBytes
Manager::size_bytes() {
    This::ensure_initialized();
    Manager& instance = *This::instance_sf;
    return instance.sizeBytes_f;
}

/* static */
SizeBytes
Manager::used_bytes() {
    This::ensure_initialized();
    Manager& instance = *This::instance_sf;
    return instance.headOffset_f;
}

/* static */
SizeBytes
Manager::free_bytes() {
    This::ensure_initialized();
    Manager& instance = *This::instance_sf;
    return instance.sizeBytes_f - instance.headOffset_f;
}

/* static */
bool
Manager::is_sealed() {
    This::ensure_initialized();
    return This::instance_sf->isSealed_f;
}

/* static */
bool
Manager::is_initialized() {
    return This::instance_sf != nullptr;
}

/* static */
void
Manager::ensure_initialized() {
    if (This::instance_sf == nullptr) {
        pican::panic("MemoryManager not initialized!");
    }
}

/* static */
void
Manager::ensure_not_sealed() {
    This::ensure_initialized();
    if (This::instance_sf->isSealed_f) {
        pican::panic("MemoryManager is sealed!");
    }
}

/* static */
Address
Manager::head_address() {
    This::ensure_initialized();
    Manager& instance = *This::instance_sf;
    return instance.address_f + instance.headOffset_f;
}

}  // namespace pican::mem
