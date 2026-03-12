#pragma once

#include <string_view>

#include "pican/Types.hpp"

namespace pican {
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
    has_next() const&;

    [[nodiscard]]
    std::string_view
    next() const&;
};
}  // namespace pican
