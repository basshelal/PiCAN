#pragma once

#include <cstddef>
#include <cstdint>

#include "pican/Types.hpp"
#include "pican/log/Utils.hpp"

namespace pican::config {

constexpr SizeBytes MEMORY_SIZE_BYTES = 400 * 1'024 * 1'024;
constexpr Count THREADS_COUNT = 8;
constexpr Milliseconds MAIN_LOOP_SLEEP_MILLISECONDS = 1'000;

// TODO @basshelal Fri 20-Feb-2026 : Make this use the can::InterfaceName type, but ensure header deps are ok
constexpr std::string_view CAN_INTERFACE_NAME = "vcan0";
constexpr SizeBytes CAN_LINUX_BUFFER_SIZE = 2 * 1'024 * 1'024; // 2 MiB
constexpr std::uint8_t CAN_THREAD_TIMEOUT_SECONDS = 5;

}  // namespace pican::config
