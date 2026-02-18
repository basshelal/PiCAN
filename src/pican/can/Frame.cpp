#include "pican/can/Frame.hpp"

#include <linux/can.h>

namespace pican::can {
void
Frame::set_from_linux_can_frame(const LinuxCanFrame& linuxFrame) & {
    this->id_f = linuxFrame.can_id;
    this->length_f = ::std::min(static_cast<std::size_t>(linuxFrame.len), FRAME_DATA_SIZE);
    for (std::size_t i = 0; i < this->length_f; ++i) {
        this->data_f[i] = linuxFrame.data[i];
    }
}

/* static */
Frame
Frame::from_linux_can_frame(const LinuxCanFrame& linuxFrame) {
    Frame frame{};
    frame.set_from_linux_can_frame(linuxFrame);
    return frame;
}
}  // namespace pican::can
