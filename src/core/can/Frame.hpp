#pragma once

#include <array>
#include <cstring>
#include <sstream>

#include <fmt/format.h>
#include <linux/can.h>

#include "core/Types.hpp"

namespace core::can {
constexpr Size FRAME_DATA_SIZE = 8;

using FrameID = canid_t;
using FrameData = std::array<UInt8, FRAME_DATA_SIZE>;

class Frame {
public:  // fields
    core::can::FrameID id_f;
    UInt8 length;
    core::can::FrameData data_f;

public:  // functions
    inline void
    set_from_linux_can_frame(const struct can_frame& linuxFrame) {
        this->id_f = linuxFrame.can_id;
        this->length = ::std::min(static_cast<Size>(linuxFrame.len), FRAME_DATA_SIZE);
        for (Size i = 0; i < this->length; ++i) {
            this->data_f[i] = linuxFrame.data[i];
        }
    }

public:  // friends
    friend std::string
    to_string(const Frame& frame) {
        std::stringstream ss;
        ss << "Frame: " << fmt::format("0x{:04X}", frame.id_f) << " : [";
        for (Size i = 0; i < frame.length; ++i) {
            ss << fmt::format("{:02X}", frame.data_f[i]);
            if (i != static_cast<Size>(frame.length) - 1) {
                ss << ", ";
            }
        }
        ss << "]";
        return ss.str();
    }
};
}  // namespace core::can
