#pragma once

#include <chrono>
#include <cstdint>

#include "pican/Types.hpp"

namespace pican {

enum class TimerUnit : std::uint8_t {
    MILLISECONDS,
    NANOSECONDS,
};

template<TimerUnit Unit_TV>
struct TimerUnitTraits;

template<>
struct TimerUnitTraits<TimerUnit::MILLISECONDS> {
    using Duration = std::chrono::milliseconds;
};

template<>
struct TimerUnitTraits<TimerUnit::NANOSECONDS> {
    using Duration = std::chrono::nanoseconds;
};

template<TimerUnit Unit_TV>
class Timer {
public:  // types
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = typename TimerUnitTraits<Unit_TV>::Duration;
    using DurationUnit = typename Duration::rep;

private:  // fields
    TimePoint startTime_f;
    TimePoint endTime_f;

public:  // constructor
    Timer() : startTime_f{}, endTime_f{} {
    }

public:  // lifetime
    Timer(const Timer& rhs) = default;

    Timer(Timer&& rhs) noexcept = default;

    Timer&
    operator=(const Timer& rhs) & = default;

    Timer&
    operator=(Timer&& rhs) & noexcept = default;

    ~Timer() = default;

public:  // member functions
    [[nodiscard]]
    TimePoint
    now() const& {
        return Clock::now();
    }

    void
    start() & {
        this->startTime_f = this->now();
    }

    DurationUnit
    stop() & {
        this->endTime_f = this->now();
        return this->duration();
    }

    [[nodiscard]]
    DurationUnit
    duration() const& {
        return std::chrono::duration_cast<Duration>(this->endTime_f - this->startTime_f).count();
    }
};

}  // namespace pican
