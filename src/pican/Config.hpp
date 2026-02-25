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

// TODO @basshelal Mon 23-Feb-2026 : We should try to make these configurable, could be from a CMake build flag/option
//  or a runtime environment flag or something like that, these are application runtime constants, who and when
//  they are set is the issue, for example the can interface of choice should be an application config variable
//
constexpr std::string_view CAN_INTERFACE_NAME = "vcan0";
constexpr SizeBytes CAN_LINUX_BUFFER_SIZE = 2 * 1'024 * 1'024; // 2 MiB
constexpr std::uint8_t CAN_THREAD_TIMEOUT_SECONDS = 5;
constexpr Count CPU_CORES_COUNT = 2;

}  // namespace pican::config
