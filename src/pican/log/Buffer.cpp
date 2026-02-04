#include "Buffer.hpp"

namespace pican::log {

Buffer::Buffer(const ThreadId& threadId, Count entriesCount) :
    threadId_f{threadId},
    entries_f{RingBuffer<Entry>{
        pican::mem::Manager::get_array<Entry>(entriesCount), RingBufferOverflowBehavior::OVERWRITE_OLDEST
    }} {
}

}  // namespace pican::log
