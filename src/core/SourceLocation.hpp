#pragma once
#include "Types.hpp"
#include "fmt/format.h"

class SourceLocation {
private:  // fields
    UInt32 line_f;
    const char* file_f;
    const char* function_f;

public:  // constructors
    explicit SourceLocation(const UInt32 line, const char* file, const char* function) :
        line_f(line), file_f(file), function_f(function) {
    }

public:  // copy-control
    SourceLocation(const SourceLocation& rhs) = default;

    SourceLocation(SourceLocation&& rhs) noexcept = default;

    SourceLocation&
    operator=(const SourceLocation& rhs) = default;

    SourceLocation&
    operator=(SourceLocation&& rhs) noexcept = default;

    ~SourceLocation() = default;

public:  // functions
    [[nodiscard]]
    inline std::string
    file() const {
        return this->file_f;
    }

    [[nodiscard]]
    inline Size
    line() const {
        return this->line_f;
    }

    [[nodiscard]]
    inline std::string
    function() const {
        return this->function_f;
    }

    [[nodiscard]]
    inline std::string
    format() const {
        return fmt::format("{}::{}:{}", this->file_f, this->function_f, this->line_f);
    }
};

#define CURRENT_SOURCE_LOCATION SourceLocation(__LINE__, __FILE__, __FUNCTION__)
