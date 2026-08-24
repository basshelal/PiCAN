// #include <string_view>
//
// #include "../src/pican/ds/RingBuffer.cppm"
// #include "test/Heap.hpp"
// #include "test/TestUtils.hpp"
// #include "test/Tracked.hpp"
//
// #define TEST_SUITE_NAME Array
//
// namespace pican {
//
// using Element = Tracked<std::string_view>;
// constexpr SizeBytes ELEMENT_SIZE = sizeof(Element);
//
// TEST(initialize_by_copy) {
//     const Count count = 8;
//     const SizeBytes blockSize = ELEMENT_SIZE * count;
//     std::byte bytes[blockSize];
//     mem::Block block{&bytes, blockSize};
//
//     Element defaultElement{"default"};
//
//     Array<Element> array = Array<Element>::initialize_by_copy(block, defaultElement);
//
//     ASSERT_EQUAL(count, array.length());
//     ASSERT_EQUAL(block.address(), array.block().address());
//     ASSERT_EQUAL(block.size_bytes(), array.block().size_bytes());
//     ASSERT_EQUAL(count - 1, array.last_index());
//
//     for (Index i = 0; i < count; ++i) {
//         Element* elementFromBlock = block.ptr_at_offset<Element>(i * ELEMENT_SIZE);
//         ASSERT_NOT_NULL(elementFromBlock);
//         ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, elementFromBlock->lastOperation);
//         ASSERT_EQUAL(1, elementFromBlock->copyCount);
//         ASSERT_EQUAL(0, elementFromBlock->moveCount);
//
//         Element* elementFromArray = array.get_ptr(i);
//         ASSERT_EQUAL(elementFromBlock, elementFromArray);
//         ASSERT_EQUAL(LifetimeOperation::COPY_CONSTRUCTOR, elementFromArray->lastOperation);
//         ASSERT_EQUAL(1, elementFromArray->copyCount);
//         ASSERT_EQUAL(0, elementFromArray->moveCount);
//         ASSERT_EQUAL(elementFromBlock->data, elementFromArray->data);
//
//         ASSERT_EQUAL(defaultElement.data, elementFromArray->data);
//     }
// }
//
// TEST(initialize_emplace) {
//     const Count count = 8;
//     const SizeBytes blockSize = ELEMENT_SIZE * count;
//     std::byte bytes[blockSize];
//     mem::Block block{&bytes, blockSize};
//
//     std::string_view data{"element"};
//
//     Array<Element> array = Array<Element>::initialize_emplace(block, data);
//
//     ASSERT_EQUAL(count, array.length());
//     ASSERT_EQUAL(block.address(), array.block().address());
//     ASSERT_EQUAL(block.size_bytes(), array.block().size_bytes());
//     ASSERT_EQUAL(count - 1, array.last_index());
//
//     for (Index i = 0; i < count; ++i) {
//         Element* elementFromBlock = block.ptr_at_offset<Element>(i * ELEMENT_SIZE);
//         ASSERT_NOT_NULL(elementFromBlock);
//         ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, elementFromBlock->lastOperation);
//         ASSERT_EQUAL(0, elementFromBlock->copyCount);
//         ASSERT_EQUAL(0, elementFromBlock->moveCount);
//
//         Element* elementFromArray = array.get_ptr(i);
//         ASSERT_EQUAL(elementFromBlock, elementFromArray);
//         ASSERT_EQUAL(LifetimeOperation::CONSTRUCTOR, elementFromArray->lastOperation);
//         ASSERT_EQUAL(0, elementFromArray->copyCount);
//         ASSERT_EQUAL(0, elementFromArray->moveCount);
//         ASSERT_EQUAL(elementFromBlock->data, elementFromArray->data);
//
//         ASSERT_EQUAL(data, elementFromArray->data);
//     }
// }
//
// TEST(set_and_get_in_bounds) {
//     const Count count = 8;
//     const SizeBytes blockSize = ELEMENT_SIZE * count;
//     std::byte bytes[blockSize];
//     mem::Block block{&bytes, blockSize};
//
//     std::string_view data{"element"};
//
//     Array<Element> array = Array<Element>::initialize_emplace(block, data);
//
//     Element element0{"element0"};
//     array.set_copy(0, element0);
//
//     const Element& gotElement0 = array.get(0);
//     ASSERT_EQUAL(element0.data, gotElement0.data);
//     ASSERT_EQUAL(element0.moveCount, 0);
//     ASSERT_EQUAL(element0.copyCount, 0);
//     ASSERT_EQUAL(gotElement0.moveCount, 0);
//     ASSERT_EQUAL(gotElement0.copyCount, 1);
//     ASSERT_EQUAL(gotElement0.lastOperation, LifetimeOperation::COPY_ASSIGNMENT);
// }
//
// TEST(iterator) {
//     const Count count = 8;
//     const SizeBytes blockSize = ELEMENT_SIZE * count;
//     std::byte bytes[blockSize];
//     mem::Block block{&bytes, blockSize};
//
//     std::array<std::array<char, 16>, count> stringArray;
//     for (std::array<char, 16> &string:stringArray) {
//         string=11;
//     }
//
//
//     for (Index i = 0; i < array.length(); ++i) {
//
//     }
//
//     Array<Element> array = Array<Element>::initialize_emplace(block, "element");
//     for (Index i = 0; i < array.length(); ++i) {
//         Element* ptr = array.get_ptr(i);
//         std::string string = fmt::format("element{}", i);
//         ptr->data = std::string_view(string.data());
//     }
//
//     for (Index i = 0; i < array.length(); ++i) {
//         Element& element = array.get(i);
//         element.callbacks.onDestructor = [&destructorsCalled, i](const Element&) -> void {
//             destructorsCalled[i] = true;
//         };
//     }
// }
//
// TEST(lifetime) {
//     const Count count = 8;
//     const SizeBytes blockSize = ELEMENT_SIZE * count;
//     std::byte bytes[blockSize];
//     mem::Block block{&bytes, blockSize};
//
//     std::array<bool, count> destructorsCalled;
//     destructorsCalled.fill(false);
//
//     for (Index i = 0; i < count; ++i) {
//         ASSERT_FALSE(destructorsCalled[i]);
//     }
//
//     {
//         Array<Element> array = Array<Element>::initialize_emplace(block, "element");
//         for (Index i = 0; i < array.length(); ++i) {
//             Element& element = array.get(i);
//             element.callbacks.onDestructor = [&destructorsCalled, i](const Element&) -> void {
//                 destructorsCalled[i] = true;
//             };
//         }
//     }
//
//     for (Index i = 0; i < count; ++i) {
//         ASSERT_TRUE(destructorsCalled[i]);
//     }
// }
//
// }  // namespace pican
