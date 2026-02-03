#include "pican/memory/Block.hpp"

namespace pican::memory {
const Block Block::NULL_BLOCK = Block{NULL_ADDRESS, UNKNOWN_SIZE};
}  // namespace pican::memory
