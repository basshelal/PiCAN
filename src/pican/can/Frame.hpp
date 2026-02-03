#pragma once

#include <cstdint>
#include <sstream>

#include <fmt/format.h>

namespace pican::can {
constexpr std::size_t FRAME_DATA_SIZE = 8;

using FrameID = canid_t;
using FrameData = std::array<std::uint8_t, FRAME_DATA_SIZE>;

class Frame {
public:  // fields
    pican::can::FrameID id_f;
    std::uint8_t length;
    pican::can::FrameData data_f;

public:  // functions
    void
    set_from_linux_can_frame(const struct can_frame& linuxFrame) {
        this->id_f = linuxFrame.can_id;
        this->length = ::std::min(static_cast<std::size_t>(linuxFrame.len), FRAME_DATA_SIZE);
        for (std::size_t i = 0; i < this->length; ++i) {
            this->data_f[i] = linuxFrame.data[i];
        }
    }

public:  // friends
    friend std::string
    to_string(const Frame& frame) {
        std::stringstream ss;
        ss << "Frame: " << fmt::format("0x{:04X}", frame.id_f) << " : [";
        for (std::size_t i = 0; i < frame.length; ++i) {
            ss << fmt::format("{:02X}", frame.data_f[i]);
            if (i != static_cast<std::size_t>(frame.length) - 1) {
                ss << ", ";
            }
        }
        ss << "]";
        return ss.str();
    }
};
}  // namespace pican::can
