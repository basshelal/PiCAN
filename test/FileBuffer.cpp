#include "pican/FileBuffer.hpp"

#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME FileBuffer

namespace pican {

TEST(create) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    ASSERT_EQUAL(block.size_bytes(), buffer.capacity_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes(), buffer.writable_bytes());
    ASSERT_EQUAL(0, buffer.readable_bytes());
    ASSERT_EQUAL(0, buffer.write_index());
    ASSERT_EQUAL(0, buffer.read_index());
}

TEST(write_read_clear) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 16;
    char dataToWrite[dataToWriteSize] = "Hello World!";

    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);

    char readIntoBuffer[dataToWriteSize];
    SizeBytes read = buffer.read_into(readIntoBuffer, sizeof(readIntoBuffer));
    ASSERT_EQUAL(read, wrote);

    const std::string_view wroteString{dataToWrite};
    const std::string_view readString{readIntoBuffer};
    ASSERT_EQUAL(wroteString, readString);
    ASSERT_EQUAL(wroteString.length(), readString.length());
    ASSERT_NOT_EQUAL(wroteString.data(), readString.data());

    ASSERT_EQUAL(wrote, buffer.read_index());
    ASSERT_EQUAL(wrote, buffer.write_index());

    buffer.clear();
    ASSERT_EQUAL(0, buffer.read_index());
    ASSERT_EQUAL(0, buffer.write_index());

    ASSERT_EQUAL(0, buffer.readable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes(), buffer.writable_bytes());
}

TEST(write_less_than_capacity) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 16;
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize < buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);
    ASSERT_EQUAL(buffer.capacity_bytes() - wrote, buffer.writable_bytes());
    ASSERT_EQUAL(wrote, buffer.readable_bytes());
    ASSERT_EQUAL(wrote, buffer.write_index());
    ASSERT_EQUAL(0, buffer.read_index());
}

TEST(write_equal_to_capacity) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = sizeof(array);
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize == buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);
    ASSERT_EQUAL(buffer.capacity_bytes() - wrote, buffer.writable_bytes());
    ASSERT_EQUAL(wrote, buffer.readable_bytes());
    ASSERT_EQUAL(wrote, buffer.write_index());
    ASSERT_EQUAL(0, buffer.read_index());
}

TEST(write_greater_than_capacity) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 512;
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize > buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(buffer.capacity_bytes(), wrote);
    ASSERT_EQUAL(0, buffer.writable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes(), buffer.readable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes(), buffer.write_index());
    ASSERT_EQUAL(0, buffer.read_index());
}

TEST(read_less_than_readable) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 128;
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize < buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);

    const SizeBytes dataToReadSize = 64;
    ASSERT_TRUE(dataToReadSize < dataToWriteSize);
    char readIntoBuffer[dataToReadSize];

    SizeBytes read = buffer.read_into(readIntoBuffer, sizeof(readIntoBuffer));

    ASSERT_EQUAL(dataToReadSize, read);
    ASSERT_EQUAL(wrote - read, buffer.readable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes() - wrote, buffer.writable_bytes());
    ASSERT_EQUAL(read, buffer.read_index());
    ASSERT_EQUAL(wrote, buffer.write_index());
}

TEST(read_equal_to_readable) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 128;
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize < buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);

    const SizeBytes dataToReadSize = 128;
    ASSERT_EQUAL(dataToReadSize, dataToWriteSize);
    char readIntoBuffer[dataToReadSize];

    SizeBytes read = buffer.read_into(readIntoBuffer, sizeof(readIntoBuffer));

    ASSERT_EQUAL(dataToReadSize, read);
    ASSERT_EQUAL(wrote - read, buffer.readable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes() - wrote, buffer.writable_bytes());
    ASSERT_EQUAL(read, buffer.read_index());
    ASSERT_EQUAL(wrote, buffer.write_index());
}

TEST(read_greater_than_readable) {
    char array[256];
    mem::Block block{array, sizeof(array)};
    FileBuffer buffer{block};

    const SizeBytes dataToWriteSize = 128;
    char dataToWrite[dataToWriteSize];
    ASSERT_TRUE(dataToWriteSize < buffer.capacity_bytes());
    SizeBytes wrote = buffer.write_from(dataToWrite, dataToWriteSize);

    ASSERT_EQUAL(dataToWriteSize, wrote);

    const SizeBytes dataToReadSize = 256;
    ASSERT_TRUE(dataToReadSize > dataToWriteSize);
    char readIntoBuffer[dataToReadSize];

    SizeBytes read = buffer.read_into(readIntoBuffer, sizeof(readIntoBuffer));

    ASSERT_EQUAL(dataToWriteSize, read);
    ASSERT_EQUAL(wrote - read, buffer.readable_bytes());
    ASSERT_EQUAL(buffer.capacity_bytes() - wrote, buffer.writable_bytes());
    ASSERT_EQUAL(read, buffer.read_index());
    ASSERT_EQUAL(wrote, buffer.write_index());
}

}  // namespace pican
