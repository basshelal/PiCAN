#include "pican/File.hpp"

#include <pican/mem/Manager.hpp>

#include "test/TestUtils.hpp"

#define TEST_SUITE_NAME File

namespace pican {

constexpr FilePath TEST_FILE_PATH = "./test-file.txt";

namespace {

File&
non_existent_file() {
    static File file{"./non-existent-test-file"};
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
existent_empty_file() {
    static File file{"./empty-test-file"};
    file.open(FileMode::WRITE_ONLY, true);
    file.sync();
    file.clear();
    file.sync();
    const Result<SizeBytes, File::Error> sizeResult = file.total_size_bytes();
    assert(sizeResult.success_value_or_panic() == 0);
    assert(file.exists());
    assert(file.is_open());
    file.close();
    assert(!file.is_open());
    return file;
}

constexpr SizeBytes EXISTENT_FILE_SIZE = 2'048;

File&
existent_filled_file() {
    static File file{"./filled-test-file"};
    file.open(FileMode::WRITE_ONLY, true);
    file.clear();
    char data[EXISTENT_FILE_SIZE];
    std::memset(data, 0, EXISTENT_FILE_SIZE);
    const Result<SizeBytes, File::Error> writeResult = file.unbuffered_write_from(data, sizeof(data));
    assert(writeResult.is_success());
    assert(writeResult.success_value_or_panic() == sizeof(data));
    const Result<SizeBytes, File::Error> sizeResult = file.total_size_bytes();
    assert(sizeResult.success_value_or_panic() == sizeof(data));
    file.sync();
    file.close();
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
    const File& file = existent_empty_file();

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
    File& file = existent_empty_file();

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

TEST(close_opened_file) {
    File& file = existent_empty_file();
    const File::SimpleResult openResult = file.open(FileMode::READ_ONLY, false);

    ASSERT_TRUE(openResult.is_success());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close().is_success());
    ASSERT_FALSE(file.is_open());
}

TEST(close_closed_file) {
    File& file = existent_empty_file();

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

TEST(remove_existent_closed_file) {
    File& file = existent_empty_file();

    ASSERT_FALSE(file.is_open());
    ASSERT_TRUE(file.exists());

    ASSERT_TRUE(file.remove().is_success());
    ASSERT_FALSE(file.exists());
}

TEST(remove_existent_opened_file) {
    File& file = existent_empty_file();

    ASSERT_TRUE(file.open(FileMode::READ_ONLY, false).is_success());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.remove().is_success());
    ASSERT_FALSE(file.is_open());
    ASSERT_FALSE(file.exists());
}

TEST(total_size_bytes) {
    File& file = existent_filled_file();

    const Result<SizeBytes, File::Error> sizeBytesResult = file.total_size_bytes();
    ASSERT_TRUE(sizeBytesResult.is_success());
    ASSERT_EQUAL(EXISTENT_FILE_SIZE, sizeBytesResult.success_value_or_panic());

    ASSERT_TRUE(file.open(FileMode::WRITE_ONLY, false).is_success());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.clear().is_success());
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.exists());

    ASSERT_TRUE(file.sync().is_success());

    const Result<SizeBytes, File::Error> newSizeBytesResult = file.total_size_bytes();
    ASSERT_TRUE(newSizeBytesResult.is_success());
    ASSERT_EQUAL(0, newSizeBytesResult.success_value_or_panic());
}

TEST(write_with_null_buffer) {
    File& file = existent_empty_file();

    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.write_from(mem::Block::NULL_BLOCK).is_success());
}

TEST(unbuffered_write_no_file_buffer) {
    File& file = existent_empty_file();

    file.remove_write_buffer();
    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_FALSE(file.has_write_buffer());

    char dataToWrite[1'024];

    const Result<SizeBytes, File::Error> writeResult = file.unbuffered_write_from(dataToWrite, sizeof(dataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());
}

TEST(unbuffered_write_with_file_buffer) {
    File& file = existent_empty_file();

    char b[128];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_write_buffer());

    char dataToWrite[1'024];

    const Result<SizeBytes, File::Error> writeResult = file.unbuffered_write_from(dataToWrite, sizeof(dataToWrite));
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(sizeof(dataToWrite), writeResult.success_value_or_panic());
}

TEST(buffered_write_no_file_buffer) {
    File& file = existent_empty_file();

    file.remove_write_buffer();
    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_FALSE(file.has_write_buffer());

    const SizeBytes bytesToWrite = 1'024;
    char dataToWrite[bytesToWrite];

    const Result<SizeBytes, File::Error> writeResult = file.write_from(dataToWrite, bytesToWrite);
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(bytesToWrite, writeResult.success_value_or_panic());

    const Result<SizeBytes, File::Error> sizeResult = file.total_size_bytes();
    ASSERT_TRUE(sizeResult.is_success());
    const SizeBytes size = sizeResult.success_value_or_panic();
    ASSERT_EQUAL(bytesToWrite, size);
}

TEST(buffered_write_with_file_buffer_large_write) {
    File& file = existent_empty_file();

    char b[128];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_write_buffer());

    const SizeBytes bytesToWrite = 1'024;
    char dataToWrite[bytesToWrite];
    ASSERT_TRUE(bytesToWrite > writeBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> writeResult = file.write_from(dataToWrite, bytesToWrite);
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(bytesToWrite, writeResult.success_value_or_panic());

    ASSERT_TRUE(file.flush().is_success());

    const Result<SizeBytes, File::Error> sizeResult = file.total_size_bytes();
    ASSERT_TRUE(sizeResult.is_success());
    const SizeBytes size = sizeResult.success_value_or_panic();
    ASSERT_EQUAL(bytesToWrite, size);
}

TEST(buffered_write_with_buffer_small_write) {
    File& file = existent_empty_file();

    char b[1'024];
    mem::Block writeBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_write_buffer(writeBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::WRITE_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_write_buffer());

    const SizeBytes bytesToWrite = 256;
    char smallDataToWrite[bytesToWrite];
    ASSERT_TRUE(bytesToWrite < writeBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> writeResult = file.write_from(smallDataToWrite, bytesToWrite);
    ASSERT_TRUE(writeResult.is_success());
    ASSERT_EQUAL(bytesToWrite, writeResult.success_value_or_panic());

    ASSERT_TRUE(file.flush().is_success());

    const Result<SizeBytes, File::Error> sizeResult = file.total_size_bytes();
    ASSERT_TRUE(sizeResult.is_success());
    const SizeBytes size = sizeResult.success_value_or_panic();
    ASSERT_EQUAL(bytesToWrite, size);
}

TEST(read_with_null_buffer) {
    File& file = existent_filled_file();

    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());

    mem::Block nullBlock{mem::Block::NULL_BLOCK};
    ASSERT_FALSE(file.read_into(nullBlock).is_success());
}

TEST(unbuffered_read_no_file_buffer) {
    File& file = existent_filled_file();

    file.remove_read_buffer();
    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.has_read_buffer());

    const SizeBytes bytesToRead = 128;
    char dataToRead[bytesToRead];
    std::memset(dataToRead, 69, bytesToRead);

    const Result<SizeBytes, File::Error> readResult = file.unbuffered_read_into(dataToRead, bytesToRead);
    ASSERT_TRUE(readResult.is_success());
    ASSERT_EQUAL(bytesToRead, readResult.success_value_or_panic());

    for (Index i = 0; i < bytesToRead; ++i) {
        ASSERT_EQUAL(0, dataToRead[i]);
    }
}

TEST(unbuffered_read_with_file_buffer) {
    File& file = existent_filled_file();

    char b[128];
    mem::Block readBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_read_buffer(readBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_read_buffer());

    const SizeBytes bytesToRead = 128;
    char dataToRead[bytesToRead];
    std::memset(dataToRead, 69, bytesToRead);

    const Result<SizeBytes, File::Error> readResult = file.unbuffered_read_into(dataToRead, bytesToRead);
    ASSERT_TRUE(readResult.is_success());
    ASSERT_EQUAL(bytesToRead, readResult.success_value_or_panic());

    for (Index i = 0; i < bytesToRead; ++i) {
        ASSERT_EQUAL(0, dataToRead[i]);
    }
}

TEST(buffered_read_no_file_buffer) {
    File& file = existent_filled_file();

    file.remove_read_buffer();
    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.has_read_buffer());

    const SizeBytes bytesToRead = 128;
    char dataToRead[bytesToRead];
    std::memset(dataToRead, 69, bytesToRead);

    const Result<SizeBytes, File::Error> readResult = file.read_into(dataToRead, bytesToRead);
    ASSERT_TRUE(readResult.is_success());
    ASSERT_EQUAL(bytesToRead, readResult.success_value_or_panic());

    for (Index i = 0; i < bytesToRead; ++i) {
        ASSERT_EQUAL(0, dataToRead[i]);
    }
}

TEST(buffered_read_with_file_buffer_large) {
    File& file = existent_filled_file();

    char b[128];
    mem::Block readBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_read_buffer(readBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_read_buffer());

    const SizeBytes bytesToRead = 1'024;
    char dataToRead[bytesToRead];
    std::memset(dataToRead, 69, bytesToRead);
    ASSERT_TRUE(bytesToRead > readBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> readResult = file.read_into(dataToRead, bytesToRead);
    ASSERT_TRUE(readResult.is_success());
    ASSERT_EQUAL(bytesToRead, readResult.success_value_or_panic());

    for (Index i = 0; i < bytesToRead; ++i) {
        ASSERT_EQUAL(0, dataToRead[i]);
    }
}

TEST(buffered_read_with_file_buffer_small) {
    File& file = existent_filled_file();

    char b[1'024];
    mem::Block readBufferBlock{b, sizeof(b)};

    const File::SimpleResult setBufferResult = file.set_read_buffer(readBufferBlock);
    ASSERT_TRUE(setBufferResult.is_success());

    file.open(FileMode::READ_ONLY, false);
    ASSERT_TRUE(file.is_open());
    ASSERT_TRUE(file.has_read_buffer());
    // ASSERT_EQUAL(0, file.current_offset());

    const SizeBytes bytesToRead = 256;
    char dataToRead[bytesToRead];
    std::memset(dataToRead, 69, bytesToRead);
    ASSERT_TRUE(bytesToRead < readBufferBlock.size_bytes());

    const Result<SizeBytes, File::Error> readResult = file.read_into(dataToRead, bytesToRead);
    ASSERT_TRUE(readResult.is_success());
    ASSERT_EQUAL(bytesToRead, readResult.success_value_or_panic());

    for (Index i = 0; i < bytesToRead; ++i) {
        ASSERT_EQUAL(0, dataToRead[i]);
    }
}

TEST(seek_to_with_buffer) {
    GTEST_SKIP();
}

TEST(file_type_regular_file) {
    File& file = existent_empty_file();
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

TEST(large_buffered_read_and_large_buffered_write) {
    char readBufferArray[1'024];
    mem::Block fileReadBuffer{readBufferArray, sizeof(readBufferArray)};

    File randomFile{"/dev/random"};
    ASSERT_TRUE(randomFile.set_read_buffer(fileReadBuffer).is_success());
    ASSERT_TRUE(randomFile.has_read_buffer());

    ASSERT_TRUE(randomFile.open(FileMode::READ_ONLY, false).is_success());
    ASSERT_TRUE(randomFile.is_open());

    char writeBufferArray[1'024];
    mem::Block fileWriteBuffer{writeBufferArray, sizeof(writeBufferArray)};

    File fileToWrite{"./test-file"};
    ASSERT_TRUE(fileToWrite.set_write_buffer(fileWriteBuffer).is_success());
    ASSERT_TRUE(fileToWrite.has_write_buffer());

    ASSERT_TRUE(fileToWrite.open(FileMode::WRITE_ONLY, true).is_success());
    ASSERT_TRUE(fileToWrite.is_open());
    ASSERT_TRUE(fileToWrite.clear().is_success());
    const Result<SizeBytes, File::Error> sizeResult = fileToWrite.total_size_bytes();
    ASSERT_TRUE(sizeResult.is_success());
    ASSERT_EQUAL(0, sizeResult.success_value_or_panic());

    const SizeBytes bytesToWrite = 8'192;
    mem::Block fileDataBlock = mem::Manager::get_block(bytesToWrite);
    const SizeBytes readBufferSize = 128;
    char readBuffer[readBufferSize];

    SizeBytes bytesWritten = 0;
    while (bytesWritten < bytesToWrite) {
        const Result<SizeBytes, File::Error> randomReadResult = randomFile.read_into(readBuffer, readBufferSize);
        ASSERT_TRUE(randomReadResult.is_success());
        const SizeBytes randomReadBytes = randomReadResult.success_value_or_panic();
        ASSERT_EQUAL(readBufferSize, randomReadBytes);

        const Result<SizeBytes, File::Error> writeResult = fileToWrite.write_from(readBuffer, readBufferSize);
        ASSERT_TRUE(writeResult.is_success());
        const SizeBytes wroteBytes = writeResult.success_value_or_panic();
        ASSERT_EQUAL(readBufferSize, wroteBytes);

        char* ptr = fileDataBlock.ptr_at_offset<char>(bytesWritten);
        std::memcpy(ptr, readBuffer, randomReadBytes);

        bytesWritten += randomReadBytes;
    }
    fileToWrite.sync();
    fileToWrite.close();

    File fileToRead{"./test-file"};

    ASSERT_TRUE(fileToRead.open(FileMode::READ_ONLY, false).is_success());
    ASSERT_TRUE(fileToRead.is_open());

    SizeBytes bytesRead = 0;
    while (bytesRead < bytesWritten) {
        const Result<SizeBytes, File::Error> readResult = fileToRead.read_into(readBuffer, readBufferSize);
        ASSERT_TRUE(readResult.is_success());
        const SizeBytes readBytes = readResult.success_value_or_panic();
        ASSERT_EQUAL(readBufferSize, readBytes);

        char* ptr = fileDataBlock.ptr_at_offset<char>(bytesRead);
        std::memcpy(ptr, readBuffer, readBytes);

        bytesRead += readBytes;
    }

    ASSERT_TRUE(fileToRead.remove().is_success());
}

}  // namespace pican
