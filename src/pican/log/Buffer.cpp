#include "Buffer.hpp"

namespace pican::log {

Buffer::Buffer(const Array<Entry>& array) :
    entries_f{array, RingBufferOverflowBehavior::OVERWRITE_OLDEST}, threadId_f{0} {
}

}  // namespace pican::log
