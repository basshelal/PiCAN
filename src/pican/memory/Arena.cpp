#include "pican/memory/Arena.hpp"

#include "pican/Contracts.hpp"

namespace pican::memory {

Block
Arena::get_block(SizeBytes size, Alignment alignment) & {
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
    CONTRACTS_ASSERT(pican::memory::address_is_aligned(address, alignment));
    this->headOffset_f = newHeadOffset;

    return Block{address, size};
}

void
Arena::rewind_to(Offset offset) & {
    if (offset > this->headOffset_f || offset > this->capacity_f) {
        return;
    }
    this->headOffset_f = offset;
}

void
Arena::reset() & {
    this->rewind_to(0);
}

}  // namespace pican::memory
