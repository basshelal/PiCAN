#include "pican/can/Event.hpp"

namespace pican::can {

void
Event::set_from_linux_can_frame(const LinuxCanFrame& linuxFrame) & {
    // TODO @basshelal Thu 12-Feb-2026 : Our CAN frame parsing functionality is going to be here!
}

/* static */
Event
Event::from_linux_can_frame(const LinuxCanFrame& linuxFrame) {
    Event event{};
    event.set_from_linux_can_frame(linuxFrame);
    return event;
}
}  // namespace pican::can
