#pragma once

#include <cstddef>
#include <cstdint>

namespace pican {
using Address = uintptr_t;
using SizeBytes = std::size_t;
using Alignment = std::size_t;
using Offset = std::size_t;
using Count = std::size_t;
using Index = std::size_t;

using FileDescriptor = int;
using Milliseconds = std::uint64_t;

}  // namespace pican
