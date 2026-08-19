module;

#include <string_view>

export module pican.core:StringSeparator;

import :types;

export namespace pican {
class StringSeparator {
private:  // fields
    char* dataPtr_f;
    SizeBytes size_f;
    char separator_f;
    mutable Index currentOffset_f;

public:  // constructors
    StringSeparator(char* data, SizeBytes size, char separator) :
        dataPtr_f{data}, size_f{size}, separator_f{separator}, currentOffset_f{0} {
    }

public:  // lifetime
    StringSeparator(const StringSeparator& rhs) = default;

    StringSeparator(StringSeparator&& rhs) noexcept = default;

    StringSeparator&
    operator=(const StringSeparator& rhs) & = default;

    StringSeparator&
    operator=(StringSeparator&& rhs) & noexcept = default;

    ~StringSeparator() = default;

public:  // member functions
    [[nodiscard]]
    bool
    has_next() const& {
        return this->currentOffset_f < this->size_f;
    }

    [[nodiscard]]
    std::string_view
    next() const& {
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
};
}  // namespace pican
