module;

#include <cstddef>
#include <cstdint>
#include <string_view>

export module pican.core:types;

export namespace pican {

// TODO @basshelal Fri 24-Jul-2026 : Convert (most? of) these to be "strong typedefs" aka, nice useful value classes
using Address = uintptr_t;
using SizeBytes = std::size_t;
using Alignment = std::size_t;
using Offset = std::size_t;
using Count = std::size_t;
using Index = std::size_t;

using FileDescriptor = int;
using Milliseconds = std::uint64_t;
using NanoSeconds = std::uint64_t;

using FilePath = std::string_view;

}  // namespace pican
