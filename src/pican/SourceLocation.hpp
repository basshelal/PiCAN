#pragma once

#include <cstdint>

#include <fmt/format.h>

// TODO @basshelal Sat 31-Jan-2026 : Below is not heap safe and also mostly unnecessary since we have our own
//  stacktrace, reconsider

class SourceLocation {
private:  // fields
    std::uint32_t line_f;
    const char* file_f;
    const char* function_f;

public:  // constructors
    explicit SourceLocation(const std::uint32_t line, const char* file, const char* function) :
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
    inline std::size_t
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
