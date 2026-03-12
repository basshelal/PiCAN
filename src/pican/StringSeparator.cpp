#include "pican/StringSeparator.hpp"

bool
pican::StringSeparator::has_next() const& {
    return this->currentOffset_f < this->size_f;
}

std::string_view
pican::StringSeparator::next() const& {
    char* startPtr = this->dataPtr_f + this->currentOffset_f;
    const SizeBytes remainingBytes = this->size_f - this->currentOffset_f;

    Index foundIndex = remainingBytes - 1;
    for (Index i = 0; i < remainingBytes; ++i) {
        char* ptr = startPtr + i;
        char c = *ptr;
        if (c == this->separator_f) {
            foundIndex = i;
            break;
        }
    }
    std::string_view result{startPtr, foundIndex};
    this->currentOffset_f += foundIndex + 1;

    return result;
}
