#pragma once

#include <chrono>
#include <cstddef>

#include "pican/Types.hpp"

namespace pican::time {

class DateTime {
public:  // constants
    static constexpr auto FORMAT_MINIMUM_LENGTH = sizeof("2026-02-05 09:48:34.450");

public:  // types
    using Year = std::uint16_t;
    using Month = std::uint8_t;
    using Day = std::uint8_t;
    using Hour = std::uint8_t;
    using Minute = std::uint8_t;
    using Second = std::uint8_t;
    using Millisecond = std::uint16_t;

private:  // fields
    Year year_f;
    Month month_f;
    Day day_f;
    Hour hour_f;
    Minute minute_f;
    Second second_f;
    Millisecond millisecond_f;

private:  // constructor
    DateTime(Year year, Month month, Day day, Hour hour, Minute minute, Second second, Millisecond millisecond) :
        year_f{year}, month_f{month}, day_f{day}, hour_f{hour}, minute_f{minute}, second_f{second},
        millisecond_f{millisecond} {
    }

public:  // copy-control
    DateTime(const DateTime& rhs) = default;

    DateTime(DateTime&& rhs) noexcept = default;

    DateTime&
    operator=(const DateTime& rhs) = default;

    DateTime&
    operator=(DateTime&& rhs) noexcept = default;

    ~DateTime() = default;

public:  // member functions
    void
    format_into(char* buffer, SizeBytes maxLength) const&;

public:  // getters
    [[nodiscard]]
    Year
    year() const& {
        return this->year_f;
    }

    [[nodiscard]]
    Month
    month() const& {
        return this->month_f;
    }

    [[nodiscard]]
    Day
    day() const& {
        return this->day_f;
    }

    [[nodiscard]]
    Hour
    hour() const& {
        return this->hour_f;
    }

    [[nodiscard]]
    Minute
    minute() const& {
        return this->minute_f;
    }

    [[nodiscard]]
    Second
    second() const& {
        return this->second_f;
    }

    [[nodiscard]]
    Millisecond
    millisecond() const& {
        return this->millisecond_f;
    }

public:  // static functions
    [[nodiscard]]
    static DateTime
    from_time_point(const std::chrono::system_clock::time_point& timePoint);

    [[nodiscard]]
    static DateTime
    now();
};


}  // namespace pican::time
