#pragma once

#include <array>
#include <cstdint>

#include <linux/can.h>

namespace pican::can {
constexpr std::size_t FRAME_DATA_SIZE = 8;

using FrameID = canid_t;
using FrameLength = std::uint8_t;
using FrameByte = std::uint8_t;
using FrameData = std::array<FrameByte, FRAME_DATA_SIZE>;
using LinuxCanFrame = struct can_frame;

class Frame {
private:  // fields
    FrameID id_f;
    FrameLength length_f;
    FrameData data_f;

private:  // constructors
    Frame() = default;

public:  // lifetime
    Frame(const Frame& rhs) = default;

    Frame(Frame&& rhs) noexcept = default;

    Frame&
    operator=(const Frame& rhs) & = default;

    Frame&
    operator=(Frame&& rhs) & noexcept = default;

    ~Frame() = default;

public:  // getters
    [[nodiscard]]
    inline FrameID
    id() const& {
        return this->id_f;
    }

    [[nodiscard]]
    inline FrameLength
    length() const& {
        return this->length_f;
    }

    [[nodiscard]]
    inline const FrameData&
    data() const& {
        return this->data_f;
    }

public:  // functions
    void
    set_from_linux_can_frame(const LinuxCanFrame& linuxFrame) &;

public:  // static factories
    [[nodiscard]]
    static Frame
    from_linux_can_frame(const LinuxCanFrame& linuxFrame);
public: // friends
    friend class CanThread;
};
}  // namespace pican::can
