#include "pican/time/DateTime.hpp"

#include <fmt/format.h>
#include <pican/Utils.hpp>

namespace pican::time {

namespace {
// Helper: Standard Leap Year Logic
// A year is a leap year if divisible by 4, unless divisible by 100 but not 400.
constexpr bool
is_leap(DateTime::Year year) {
    return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}
}  // namespace

void
DateTime::format_into(char* buffer, SizeBytes maxLength) const& {
    if (buffer == nullptr || maxLength == 0) {
        return;
    }
    fmt::format_to_n(
        buffer, maxLength, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", this->year_f, this->month_f, this->day_f,
        this->hour_f, this->minute_f, this->second_f, this->millisecond_f
    );
}

/* static */
DateTime
DateTime::from_time_point(const std::chrono::system_clock::time_point& timePoint) {
    auto durationSinceEpoch = timePoint.time_since_epoch();
    auto millisSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(durationSinceEpoch).count();
    auto secondsSinceEpoch = millisSinceEpoch / 1'000;

    auto millisecond = static_cast<DateTime::Millisecond>(millisSinceEpoch % 1'000);
    auto secondsOfDay = secondsSinceEpoch % 86'400;

    auto hour = static_cast<DateTime::Hour>(secondsOfDay / 3'600);
    auto secondsRemainingInHour = secondsOfDay % 3'600;
    auto minute = static_cast<DateTime::Minute>(secondsRemainingInHour / 60);
    auto second = static_cast<DateTime::Second>(secondsRemainingInHour % 60);

    // TODO @basshelal Thu 05-Feb-2026 : Clean up and understand
    // Below written by Gemini

    // --- PART 2: DATE (The No-Loop Method) ---
    // Shift epoch from 1970-01-01 to 0000-03-01.
    // Why? 0000-03-01 is a "Super Leap Cycle" boundary.
    // It simplifies the math so leap days always fall at the *end* of a cycle.
    int days = static_cast<int>(secondsSinceEpoch / 86'400);
    days += 719'468;  // Days between 0000-03-01 and 1970-01-01

    // 1. Era (400 year blocks) - 146097 days
    const int era = days / 146'097;
    days %= 146'097;  // Keep the remainder

    // 2. Century (100 year blocks) - 36524 days
    // Logic: 100 years has 24 leap years (not 25).
    // Clamp to 3 because the 4th century (leap century) is handled by Era.
    const int century = std::min(days / 36'524, 3);
    days -= century * 36'524;

    // 3. Quad (4 year blocks) - 1461 days
    // Logic: 4 years has 1 leap year.
    const int quad = days / 1'461;
    days %= 1'461;

    // 4. Year (1 year blocks) - 365 days
    // Clamp to 3 because the 4th year is the leap year (handled by Quad).
    const int y_index = std::min(days / 365, 3);
    days -= y_index * 365;

    // Calculate actual Year
    // Since we started at month 03 (March), this is temporary.
    int year = (era * 400) + (century * 100) + (quad * 4) + y_index;

    // --- PART 3: MONTH/DAY (Lookup Table) ---
    // 'days' is now the day of the year (0-365), relative to March 1st.
    // Use a lookup table for the start day of each month (from March)
    // Mar(0), Apr(31), May(61), Jun(92)...
    static const uint16_t month_starts[] = {0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337};

    // Find month: Just divide by ~30.6 (approx month length) to guess index
    // The magic number (5 * days + 2) / 153 maps exactly to the month index (0-11)
    int month_idx = (5 * days + 2) / 153;

    // Calculate final Day and Month
    int day = days - month_starts[month_idx] + 1;  // 1-indexed
    int month = month_idx + 3;                     // Shift back from March-based (3) to Jan-based (1)

    // Handle Jan/Feb rollover (they belong to the *next* numerical year in this logic)
    if (month > 12) {
        month -= 12;
        year++;
    }

    return DateTime{
        static_cast<DateTime::Year>(year),
        static_cast<DateTime::Month>(month),
        static_cast<DateTime::Day>(day),
        hour,
        minute,
        second,
        millisecond
    };
}

/* static */
DateTime
DateTime::now() {
    return DateTime::from_time_point(std::chrono::system_clock::now());
}

}  // namespace pican::time
