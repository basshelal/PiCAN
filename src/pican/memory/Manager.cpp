#include "pican/memory/Manager.hpp"

#include <sys/mman.h>

namespace pican::memory {

Manager Manager::instance_sf{};
bool Manager::initialized_sf = false;

/* static */
void
Manager::initialize(SizeBytes size) {
    if (Manager::initialized_sf) {
        return;
    }
    const SizeBytes toAllocate = pican::clamp(MIN_SIZE, size, MAX_SIZE);

    void* memory = ::mmap(nullptr, toAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // TODO @basshelal Fri 30-Jan-2026 : Below panics need to be a bit more clear and descriptive,
    //  also ensure that pica::panic is heapless
    if (memory == MAP_FAILED || memory == nullptr) {
        pican::panic("Could not allocate!");
    }

    const int result = ::mlockall(MCL_CURRENT | MCL_FUTURE);
    if (result != 0) {
        pican::panic("Could not lock all!");
    }

    Manager::initialized_sf = true;
    Manager::instance_sf.address_f = pican::memory::ptr_to_address(memory);
    Manager::instance_sf.sizeBytes_f = toAllocate;
    Manager::instance_sf.headOffset_f = 0;
    Manager::instance_sf.isSealed_f = false;
}

/* static */
Manager&
Manager::get() {
    if (!Manager::initialized_sf) {
        pican::panic("Not initialized!");
    }
    return Manager::instance_sf;
}

Block
Manager::get_block(SizeBytes size, Alignment alignment) & {
    this->ensure_not_sealed();

    const Address headAddress = this->head_address();

    const Alignment modulo = headAddress % alignment;
    SizeBytes paddingRequired = 0;
    if (modulo != 0) {
        paddingRequired = alignment - modulo;
    }

    const Offset newHeadOffset = this->headOffset_f + paddingRequired + size;
    if (newHeadOffset > this->sizeBytes_f) {
        pican::memory::panic_out_of_memory();
    }

    const Address address = headAddress + paddingRequired;
    CONTRACTS_ASSERT(pican::memory::address_is_aligned(address, alignment));
    this->headOffset_f = newHeadOffset;

    return Block{address, size};
}

Arena
Manager::get_arena(SizeBytes size, Alignment alignment) & {
    this->ensure_not_sealed();
    Block block = this->get_block(size, alignment);

    return Arena{block};
}

void
Manager::seal() & {
    this->isSealed_f = true;
}

void
Manager::ensure_not_sealed() const& {
    if (this->isSealed_f) {
        pican::panic("Cannot use more memory, MemoryManager is sealed!");
    }
}

Address
Manager::head_address() const& {
    return this->address_f + this->headOffset_f;
}

}  // namespace pican::memory
