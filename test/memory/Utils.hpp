// #pragma once
//
// #include <cstdint>
//
// inline void
// test_memory(void* address, std::size_t length) {
//     for (std::size_t i = 0; i < length; ++i) {
//         char* ptr = static_cast<char*>(address) + i;
//         [[maybe_unused]]
//         char c = *ptr;
//         *ptr = c;
//     }
// }
