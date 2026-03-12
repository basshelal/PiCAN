#pragma once

#include <cstdint>

#include "pican/Timer.hpp"
#include "pican/Types.hpp"

namespace pican::can {
struct CanInfo {
    NanoSeconds lastFrameWaitTime;
    NanoSeconds lastFrameProcessingTime;
    SizeBytes lastFrameSize;
    std::uint64_t totalFramesRead;
};
}  // namespace pican::can
