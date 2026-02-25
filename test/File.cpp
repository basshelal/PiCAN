#include "pican/File.hpp"

#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME File

namespace pican {

constexpr FilePath TEST_FILE_PATH = "./test-file.txt";

namespace {

File&
non_existent_file() {
    static File file{"./non-existent"};
    if (file.exists()) {
        file.remove();
    }
    assert(!file.exists());
    if (file.is_open()) {
        file.close();
    }
    assert(!file.is_open());
    return file;
}

File&
existent_file() {
    static File file{"./existent"};
    if (!file.exists()) {
        file.open(FileMode::READ_WRITE, true);
    }
    assert(file.exists());
    if (file.is_open()) {
        file.close();
    }
    assert(!file.is_open());
    return file;
}

}  // namespace

TEST(non_existent) {
    const File& file = non_existent_file();

    const bool exists = File::exists(file.path());
    ASSERT_FALSE(exists);
    ASSERT_FALSE(file.is_open());
}

TEST(existent) {
    const File& file = existent_file();

    const bool exists = File::exists(file.path());
    ASSERT_TRUE(exists);
    ASSERT_FALSE(file.is_open());
}

TEST(open_non_existent_no_create) {
    File& file = non_existent_file();

    ASSERT_FALSE(file.open(FileMode::READ_ONLY, false).is_success());
    ASSERT_FALSE(file.exists());
    ASSERT_FALSE(file.is_open());
}

TEST(open_non_existent_create) {
    File& file = non_existent_file();

    ASSERT_TRUE(file.open(FileMode::READ_ONLY, true).is_success());
    ASSERT_TRUE(file.exists());
    ASSERT_TRUE(file.is_open());
}

TEST(open_existent_no_create) {
    File& file = existent_file();

    ASSERT_TRUE(file.open(FileMode::READ_ONLY, false).is_success());
    ASSERT_TRUE(file.exists());
    ASSERT_TRUE(file.is_open());
}

TEST(open_existent_create) {
    File& file = non_existent_file();

    ASSERT_TRUE(file.open(FileMode::READ_ONLY, true).is_success());
    ASSERT_TRUE(file.exists());
    ASSERT_TRUE(file.is_open());
}

TEST(close_open_file) {
    File& file = existent_file();
    const File::SimpleResult openResult = file.open(FileMode::READ_ONLY);

    ASSERT_TRUE(openResult.is_success());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close().is_success());
    ASSERT_FALSE(file.is_open());
}

TEST(close_closed_file) {
    File& file = existent_file();

    ASSERT_FALSE(file.is_open());

    ASSERT_FALSE(file.close().is_success());
    ASSERT_FALSE(file.is_open());
}

TEST(close_non_existent_file) {
    File& file = non_existent_file();

    ASSERT_FALSE(file.exists());
    ASSERT_FALSE(file.is_open());

    ASSERT_FALSE(file.close().is_success());
    ASSERT_FALSE(file.is_open());
}

TEST(remove_non_existent_file) {
    File& file = non_existent_file();

    ASSERT_FALSE(file.exists());

    ASSERT_FALSE(file.remove().is_success());
    ASSERT_FALSE(file.exists());
}

TEST(remove_open_file) {
    File& file = existent_file();

    file.open();
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.remove().is_success());
    ASSERT_FALSE(file.is_open());
    ASSERT_FALSE(file.exists());
}

TEST(total_size_bytes_and_clear) {
    File& file = existent_file();

    file.open();
    ASSERT_TRUE(file.is_open());

    char dataToWrite[1'024];
    const Result<SizeBytes, File::Error> writeResult = file.write_from(dataToWrite, sizeof(dataToWrite));

    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());

    ASSERT_TRUE(file.sync().is_success());

    const Result<SizeBytes, File::Error> sizeBytesResult = file.total_size_bytes();
    ASSERT_TRUE(sizeBytesResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), sizeBytesResult.success_value_or_panic());

    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.exists());

    ASSERT_TRUE(file.sync().is_success());

    const Result<SizeBytes, File::Error> newSizeBytesResult = file.total_size_bytes();
    ASSERT_TRUE(newSizeBytesResult.is_success());
    ASSERT_EQUAL(0, newSizeBytesResult.success_value_or_panic());
}

TEST(write_with_null_buffer) {
    File& file = existent_file();

    file.open();
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.write_from(mem::Block::NULL_BLOCK).is_success());
}

TEST(unbuffered_write_no_buffer) {
    File& file = existent_file();

    file.open(FileMode::WRITE_ONLY);
    file.remove_write_buffer();
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());

    ASSERT_FALSE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char dataToWrite[1'024];

    const Result<SizeBytes, File::Error> writeResult = file.unbuffered_write_from(dataToWrite, sizeof(dataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());

    ASSERT_EQUAL(sizeof(dataToWrite), file.current_offset());
}

TEST(unbuffered_write_with_buffer) {
    File& file = existent_file();

    char b[128];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char dataToWrite[1'024];

    const Result<SizeBytes, File::Error> writeResult = file.unbuffered_write_from(dataToWrite, sizeof(dataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());

    ASSERT_EQUAL(sizeof(dataToWrite), file.current_offset());
}

TEST(buffered_write_no_buffer) {
    File& file = existent_file();

    file.remove_write_buffer();
    file.open(FileMode::WRITE_ONLY);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());

    ASSERT_FALSE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char dataToWrite[1'024];

    const Result<SizeBytes, File::Error> writeResult = file.write_from(dataToWrite, sizeof(dataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());

    ASSERT_EQUAL(sizeof(dataToWrite), file.current_offset());
}

TEST(buffered_write_with_buffer_large) {
    File& file = existent_file();

    char b[128];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char largeDataToWrite[1'024];
    ASSERT_TRUE(sizeof(largeDataToWrite) > writeBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> writeResult = file.write_from(largeDataToWrite, sizeof(largeDataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(largeDataToWrite), writeResult.success_value_or_panic());

    ASSERT_FALSE(file.has_unflushed_data());
    ASSERT_EQUAL(sizeof(largeDataToWrite), file.current_offset());
}

TEST(buffered_write_with_buffer_small) {
    File& file = existent_file();

    char b[1'024];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char smallDataToWrite[256];
    ASSERT_TRUE(sizeof(smallDataToWrite) < writeBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> writeResult = file.write_from(smallDataToWrite, sizeof(smallDataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(smallDataToWrite), writeResult.success_value_or_panic());

    ASSERT_TRUE(file.has_unflushed_data());
    ASSERT_EQUAL(0, file.current_offset());

    ASSERT_TRUE(file.flush().is_success());

    ASSERT_FALSE(file.has_unflushed_data());
    ASSERT_EQUAL(sizeof(smallDataToWrite), file.current_offset());
}

TEST(buffered_write_with_buffer_equal) {
    File& file = existent_file();

    char b[128];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.has_write_buffer());
    ASSERT_EQUAL(0, file.current_offset());

    char largeDataToWrite[128];
    ASSERT_EQUAL(sizeof(largeDataToWrite), writeBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> writeResult = file.write_from(largeDataToWrite, sizeof(largeDataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(largeDataToWrite), writeResult.success_value_or_panic());

    ASSERT_FALSE(file.has_unflushed_data());
    ASSERT_EQUAL(sizeof(largeDataToWrite), file.current_offset());
}

TEST(write_with_append_flag) {
    GTEST_SKIP();
}

TEST(unbuffered_read_no_buffer) {
    GTEST_SKIP();
}

TEST(unbuffered_read_with_buffer) {
    GTEST_SKIP();
}

TEST(buffered_read_no_buffer) {
    GTEST_SKIP();
}

TEST(buffered_read_with_buffer_large) {
    GTEST_SKIP();
}

TEST(buffered_read_with_buffer_small) {
    GTEST_SKIP();
}

TEST(buffered_read_with_buffer_equal) {
    GTEST_SKIP();
}

TEST(read_write_buffered) {
    GTEST_SKIP();
}

TEST(seek_to_with_buffer) {
    GTEST_SKIP();
}

TEST(file_type_regular_file) {
    File& file = existent_file();
    const Result<FileType, File::Error> fileTypeResult = File::file_type(file.path());

    ASSERT_TRUE(fileTypeResult.is_success());
    ASSERT_EQUAL(FileType::REGULAR_FILE, fileTypeResult.success_value_or_panic());
}

TEST(file_type_directory) {
    GTEST_SKIP();
}

TEST(file_type_block_device) {
    FilePath path{"/dev/nvme0n1"};
    if (!File::exists(path)) {
        path = "/dev/sda0n1";
    }
    if (!File::exists(path)) {
        GTEST_SKIP() << "No known block devices to test!";
    }

    const Result<FileType, File::Error> fileTypeResult = File::file_type(path);

    ASSERT_TRUE(fileTypeResult.is_success());
    ASSERT_EQUAL(FileType::BLOCK_DEVICE, fileTypeResult.success_value_or_panic());
}

TEST(file_type_fifo_pipe) {
    const FilePath path{"/tmp/pican-test-fifo"};
    ::mkfifo(path.data(), 0666);
    ASSERT_TRUE(File::exists(path));

    const Result<FileType, File::Error> fileTypeResult = File::file_type(path);

    ASSERT_TRUE(fileTypeResult.is_success());
    ASSERT_EQUAL(FileType::FIFO_PIPE, fileTypeResult.success_value_or_panic());

    ::unlink(path.data());
}

TEST(file_type_link) {
    const FilePath path{"/bin/sh"};
    const Result<FileType, File::Error> fileTypeResult = File::file_type(path);

    ASSERT_TRUE(fileTypeResult.is_success());
    ASSERT_EQUAL(FileType::LINK, fileTypeResult.success_value_or_panic());
}

TEST(file_type_socket) {
    GTEST_SKIP() << "No known sockets to test!";
}

TEST(file_type_char_device) {
    const FilePath path{"/dev/null"};
    const Result<FileType, File::Error> fileTypeResult = File::file_type(path);

    ASSERT_TRUE(fileTypeResult.is_success());
    ASSERT_EQUAL(FileType::CHAR_DEVICE, fileTypeResult.success_value_or_panic());
}

}  // namespace pican
