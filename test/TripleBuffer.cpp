// #include "../src/pican/sync/TripleBuffer.cppm"
//
// #include <thread>
//
// #include "test/TestUtils.hpp"
// #include "test/Tracked.hpp"
//
// #define TEST_SUITE_NAME TripleBuffer
//
// namespace pican {
//
// struct Data {
//     std::uint64_t value;
//     std::array<std::uint8_t, 4> unused;
// };
//
// static_assert(!std::atomic<Data>::is_always_lock_free);
//
// TEST(create) {
//     TripleBuffer<Data> tripleBuffer{};
//
//     ASSERT_FALSE(tripleBuffer.has_new());
// }
//
// TEST(test) {
//     SKIP_TEST("Fix later");
//     TripleBuffer<Data> tripleBuffer{{0}};
//     std::atomic_bool isRunning{true};
//
//     std::thread writer{[&tripleBuffer, &isRunning]() -> void {
//         std::uint64_t counter = 0;
//         while (isRunning.load(std::memory_order_acquire)) {
//             tripleBuffer.write(Data{++counter});
//             printf("W: %zu\n", counter);
//             std::this_thread::sleep_for(std::chrono::nanoseconds{std::rand() % 100});
//         }
//         fprintf(stderr, "finished write\n");
//     }};
//
//     std::thread reader{[&tripleBuffer, &isRunning]() -> void {
//         Data oldData{};
//         while (isRunning.load(std::memory_order_acquire)) {
//             if (tripleBuffer.has_new()) {
//                 const Data current = tripleBuffer.read();
//                 assert(current.value > oldData.value);
//                 printf("R: %zu -> %zu\n", oldData.value, current.value);
//                 if (current.value <= oldData.value) {
//                     fprintf(stderr, "R: %zu -> %zu\n", oldData.value, current.value);
//                 }
//                 oldData = current;
//             }
//
//             std::this_thread::sleep_for(std::chrono::nanoseconds{std::rand() % 100});
//         }
//         fprintf(stderr, "finished read\n");
//     }};
//
//     std::this_thread::sleep_for(std::chrono::seconds{3});
//     isRunning.store(false, std::memory_order_release);
//
//     writer.join();
//     reader.join();
// }
//
// }  // namespace pican
