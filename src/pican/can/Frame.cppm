module;

#include <array>
#include <cstdint>
#include <cstring>

#include <linux/can.h>

export module pican.can:Frame;

import pican.core;

export namespace pican::can {
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
    Milliseconds timestamp_f;

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
    FrameID
    id() const& {
        return this->id_f;
    }

    [[nodiscard]]
    FrameLength
    length() const& {
        return this->length_f;
    }

    [[nodiscard]]
    const FrameData&
    data() const& {
        return this->data_f;
    }

    [[nodiscard]]
    Milliseconds
    timestamp() const& {
        return this->timestamp_f;
    }

public:  // member functions
    void
    clear() & {
        std::memset(this->data_f.data(), 0, FRAME_DATA_SIZE);
    }

    void
    set_from_linux_can_frame(const LinuxCanFrame& linuxFrame) & {
        this->id_f = linuxFrame.can_id;
        this->length_f = ::std::min(static_cast<std::size_t>(linuxFrame.len), FRAME_DATA_SIZE);
        for (std::size_t i = 0; i < FRAME_DATA_SIZE; ++i) {
            if (i < linuxFrame.len) {
                this->data_f[i] = linuxFrame.data[i];
            } else {
                this->data_f[i] = 0;
            }
        }
        this->timestamp_f = pican::get_current_millis();
    }

public:  // friends
    friend class CanThread;
};
}  // namespace pican::can
