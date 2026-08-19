module;

#include <cstdint>

export module pican.can:CanInfo;

import pican.core;

export namespace pican::can {
struct CanInfo {
    NanoSeconds lastFrameWaitTime;
    NanoSeconds lastFrameProcessingTime;
    SizeBytes lastFrameSize;
    std::uint64_t totalFramesRead;
};
}  // namespace pican::can
