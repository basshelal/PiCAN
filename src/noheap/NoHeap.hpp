#pragma once

namespace noheap {
void
seal_heap();

[[nodiscard]]
bool
heap_is_sealed();
}  // namespace noheap
