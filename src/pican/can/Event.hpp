#pragma once

#include <cstdint>

#include <pican/Types.hpp>

#include "pican/can/Frame.hpp"

namespace pican::can {

enum class EventType : std::uint8_t {
    ENGINE_RPM,
    THROTTLE_LEVEL,
    STEERING_ANGLE,
    SPEED,
    BRAKES_STATUS,
    BRAKES_LEVEL,
    GEAR,
    FUEL_LEVEL,
    HANDBRAKE_STATUS,
    INDICATORS,
    HAZARD_LIGHTS,
    HEADLIGHTS,
    ODOMETER,
    WHEEL_SPEED,
    SEATBELT,
    LEFT_DOORS_STATUS,
    RIGHT_DOORS_STATUS,
    OUTSIDE_TEMP,
    REPORTED_MPG,
};

class Event {
private:  // fields
    EventType type_f;
    Milliseconds timeStamp_f;
    std::uint64_t data_f;

public:  // constructor
    Event() = default;

public:  // lifetime
    Event(const Event& rhs) = default;

    Event(Event&& rhs) noexcept = default;

    Event&
    operator=(const Event& rhs) & = default;

    Event&
    operator=(Event&& rhs) & noexcept = default;

    ~Event() = default;

public:  // getters
    [[nodiscard]]
    inline EventType
    type() const& {
        return this->type_f;
    }

    [[nodiscard]]
    inline Milliseconds
    timestamp() const& {
        return this->timeStamp_f;
    }

public:  // member functions
    void
    set_from_linux_can_frame(const LinuxCanFrame& linuxFrame) &;

public:  // static factories
    [[nodiscard]]
    static Event
    from_linux_can_frame(const LinuxCanFrame& linuxCanFrame);
};

}  // namespace pican::can
