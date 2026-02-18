#pragma once

#include "pican/Types.hpp"
#include "pican/log/Utils.hpp"

namespace pican::config {

constexpr SizeBytes MEMORY_SIZE_BYTES = 400 * 1'024 * 1'024;
constexpr Count THREADS_COUNT = 8;
constexpr Milliseconds MAIN_LOOP_SLEEP_MILLISECONDS = 1'000;

}  // namespace pican::config
