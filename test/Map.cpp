// #include "../src/pican/ds/Map.cppm"
//
// #include <string_view>
//
// #include "../src/pican/fs/File.hpp"
// #include "pican/mem/Block.cppm"
// #include "pican/mem/Manager.cppm"
// #include "test/TestUtils.hpp"
// #include "test/Tracked.hpp"
//
// #define TEST_SUITE_NAME Map
//
// namespace pican {
//
// using KeyType = std::string_view;
// using ValueType = Tracked<std::string_view>;
//
// TEST(create) {
//     const Count elementCount = 16;
//     const SizeBytes blockSize = Map<KeyType, ValueType>::ELEMENT_SIZE * elementCount;
//
//     std::byte bytes[blockSize];
//
//     mem::Block block{&bytes, blockSize};
//
//     Map<KeyType, ValueType> map{block};
//
//     ASSERT_EQUAL(elementCount, map.capacity());
//     ASSERT_EQUAL(0, map.size());
// }
//
// TEST(put_elements) {
//     const Count elementCount = 16;
//     const SizeBytes blockSize = Map<KeyType, ValueType>::ELEMENT_SIZE * elementCount;
//
//     std::byte bytes[blockSize];
//
//     mem::Block block{&bytes, blockSize};
//
//     Map<KeyType, ValueType> map{block};
//
//     ASSERT_EQUAL(elementCount, map.capacity());
//     ASSERT_EQUAL(0, map.size());
//
//     const KeyType key0{"key0"};
//     const ValueType value0{"value0"};
//
//     map.put_copy(key0, value0);
//
//     ASSERT_EQUAL(1, map.size());
//
//     const ValueType& gotValue = map.get(key0).value();
//
//     ASSERT_EQUAL(value0.data, gotValue.data);
// }
//
//
// }  // namespace pican
