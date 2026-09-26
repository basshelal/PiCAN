#pragma once

#include <cstdint>

#include "pican/core/core.hpp"

namespace pican::can {
struct CanInfo {
    NanoSeconds lastFrameWaitTime;
    NanoSeconds lastFrameProcessingTime;
    SizeBytes lastFrameSize;
    std::uint64_t totalFramesRead;
};
}  // namespace pican::can
