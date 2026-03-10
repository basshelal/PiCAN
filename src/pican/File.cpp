#include "pican/File.hpp"

#include <string_view>

#include <errno.h>
#include <fcntl.h>
#include <magic_enum/magic_enum.hpp>
#include <sys/stat.h>
#include <unistd.h>

#include "pican/Contracts.hpp"
#include "pican/File.hpp"
#include "pican/Log.hpp"

namespace pican {

using Stat = struct stat;

File::File(FilePath path) :
    path_f{path}, descriptor_f{NULL_FILE_DESCRIPTOR}, mode_f{FileMode::READ_ONLY}, isOpen_f{false}, readBuffer_f{},
    lastReadOffset_f{0}, writBuffer_f{}, lastWriteOffset_f{0} {
}

File::File(File&& rhs) noexcept :
    path_f{rhs.path_f}, descriptor_f{rhs.descriptor_f}, mode_f{rhs.mode_f}, isOpen_f{rhs.isOpen_f},
    readBuffer_f{std::move(rhs.readBuffer_f)}, lastReadOffset_f{rhs.lastReadOffset_f},
    writBuffer_f{std::move(rhs.writBuffer_f)}, lastWriteOffset_f{rhs.lastWriteOffset_f} {
    rhs.descriptor_f = NULL_FILE_DESCRIPTOR;
    rhs.isOpen_f = false;
    rhs.readBuffer_f.reset();
    rhs.writBuffer_f.reset();
}

File&
File::operator=(File&& rhs) & noexcept {
    if (std::addressof(rhs) == this) {
        return *this;
    }
    this->path_f = rhs.path_f;
    this->descriptor_f = rhs.descriptor_f;
    this->mode_f = rhs.mode_f;
    this->isOpen_f = rhs.isOpen_f;
    this->readBuffer_f = std::move(rhs.readBuffer_f);
    this->lastReadOffset_f = rhs.lastReadOffset_f;
    this->writBuffer_f = std::move(rhs.writBuffer_f);
    this->lastWriteOffset_f = rhs.lastWriteOffset_f;

    rhs.descriptor_f = NULL_FILE_DESCRIPTOR;
    rhs.isOpen_f = false;
    rhs.readBuffer_f.reset();
    rhs.writBuffer_f.reset();

    return *this;
}

File::~File() {
    this->close();
}

File::SimpleResult
File::set_read_buffer(const mem::Block& block) & {
    CONTRACTS_PRECONDITION(!block.is_null());
    if (this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_OPEN);
    }
    this->readBuffer_f.emplace(block);
    return File::SimpleResult::success_default();
}

File::SimpleResult
File::remove_read_buffer() & {
    if (this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_OPEN);
    }
    this->readBuffer_f.reset();
    CONTRACTS_ASSERT(!this->has_read_buffer());
    return File::SimpleResult::success_default();
}

bool
File::has_read_buffer() const& {
    return this->readBuffer_f.has_value();
}

File::SimpleResult
File::set_write_buffer(const mem::Block& block) & {
    CONTRACTS_PRECONDITION(!block.is_null());
    if (this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_OPEN);
    }
    this->writBuffer_f.emplace(block);
    CONTRACTS_ASSERT(this->has_write_buffer());
    return File::SimpleResult::success_default();
}

File::SimpleResult
File::remove_write_buffer() & {
    if (this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_OPEN);
    }
    this->writBuffer_f.reset();
    CONTRACTS_ASSERT(!this->has_write_buffer());
    return File::SimpleResult::success_default();
}

bool
File::has_write_buffer() const& {
    return this->writBuffer_f.has_value();
}

File::SimpleResult
File::open(FileMode mode, bool create) & {
    int flags = 0;
    const mode_t filePermissions = 0666;  // read/write permissions, used only if the file is created
    switch (mode) {
        case FileMode::READ_ONLY: {
            flags |= O_RDONLY;
            break;
        }
        case FileMode::WRITE_ONLY: {
            flags |= O_WRONLY;
            break;
        }
    }
    if (create) {
        flags |= O_CREAT;
    }

    const FileDescriptor fd = ::open(this->path_f.data(), flags, filePermissions);
    if (fd == NULL_FILE_DESCRIPTOR) {
        const int err = errno;
        const std::string_view modeString = magic_enum::enum_name(this->mode_f);
        pican::log_error("Error opening file path: {} mode: {} err: {}", this->path_f, modeString, ::strerror(err));
        switch (err) {
            case EACCES: {
                return File::SimpleResult::failure_by_copy(File::Error::PERMISSION_DENIED);
            }
            case EISDIR: {
                return File::SimpleResult::failure_by_copy(File::Error::FILE_IS_DIR);
            }
            case ENOENT: {
                return File::SimpleResult::failure_by_copy(File::Error::FILE_NOT_FOUND);
            }
            case EROFS: {
                return File::SimpleResult::failure_by_copy(File::Error::READ_ONLY_FILESYSTEM);
            }
            default: {
                return File::SimpleResult::failure_by_copy(File::Error::UNKNOWN);
            }
        }
    }
    CONTRACTS_ASSERT(fd != NULL_FILE_DESCRIPTOR);
    this->descriptor_f = fd;
    this->isOpen_f = true;
    this->mode_f = mode;
    this->lastReadOffset_f = 0;
    this->lastWriteOffset_f = 0;

    return File::SimpleResult::success_default();
}

File::SimpleResult
File::close() & {
    if (!this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    const int res = ::close(this->descriptor_f);
    if (res != 0) {
        const int err = errno;
        pican::log_error("Error closing file path: {} err: {}", this->path_f, ::strerror(err));
        switch (err) {
            case EBADF: {
                return File::SimpleResult::failure_by_copy(File::Error::FILE_NOT_FOUND);
            }
            default: {
                return File::SimpleResult::failure_by_copy(File::Error::UNKNOWN);
            }
        }
    }
    this->descriptor_f = NULL_FILE_DESCRIPTOR;
    this->isOpen_f = false;
    this->lastReadOffset_f = 0;
    this->lastWriteOffset_f = 0;
    if (this->has_write_buffer()) {
        this->writBuffer_f.value().clear();
    }
    if (this->has_read_buffer()) {
        this->readBuffer_f.value().clear();
    }

    return File::SimpleResult::success_default();
}

Result<Offset, File::Error>
File::seek_to(Offset offset) & {
    if (!this->is_seekable()) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::NOT_SEEKABLE);
    }
    if (!this->isOpen_f) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (this->can_flush()) {
        this->flush();
    }

    return this->actual_seek(offset);
}

Result<SizeBytes, File::Error>
File::write_from(const mem::Block& source) & {
    return this->write_from(source.address_to_ptr<void>(), source.size_bytes());
}

Result<SizeBytes, File::Error>
File::write_from(void* source, SizeBytes size) & {
    if (!this->isOpen_f) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (this->mode_f != FileMode::WRITE_ONLY) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::INCORRECT_MODE);
    }
    if (source == nullptr) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }
    if (!this->has_write_buffer()) {
        return this->unbuffered_write_from(source, size);
    }
    CONTRACTS_ASSERT(this->has_write_buffer());

    FileBuffer& writeBuffer = this->writBuffer_f.value();
    char* srcPtr = static_cast<char*>(source);

    SizeBytes bytesWritten = 0;
    while (bytesWritten < size) {
        const SizeBytes bytesLeftToWrite = size - bytesWritten;
        if (writeBuffer.writable_bytes() == 0) {
            this->flush();
        }
        const SizeBytes writtenBytesIntoBuffer = writeBuffer.write_from(srcPtr + bytesWritten, bytesLeftToWrite);
        bytesWritten += writtenBytesIntoBuffer;
    }

    return pican::Result<Offset, File::Error>::success_by_copy(bytesWritten);
}

Result<SizeBytes, File::Error>
File::unbuffered_write_from(const mem::Block& source) & {
    return this->unbuffered_write_from(source.address_to_ptr<void>(), source.size_bytes());
}

Result<SizeBytes, File::Error>
File::unbuffered_write_from(void* source, SizeBytes size) & {
    if (!this->isOpen_f) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (source == nullptr) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }

    // need to flush any uncommitted changes first, does nothing if there are none
    this->flush();

    const Result<SizeBytes, File::Error> writeResult = this->actual_write_from(source, size);
    if (writeResult.is_failure()) {
        return writeResult;
    }
    this->lastWriteOffset_f += writeResult.success_value_or_panic();

    return writeResult;
}

Result<SizeBytes, File::Error>
File::read_into(mem::Block& destination) const& {
    return this->read_into(destination.address_to_ptr<void>(), destination.size_bytes());
}

Result<SizeBytes, File::Error>
File::read_into(void* destination, SizeBytes size) const& {
    if (!this->isOpen_f) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (destination == nullptr) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }
    if (this->mode_f != FileMode::READ_ONLY) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::INCORRECT_MODE);
    }
    if (!this->has_read_buffer()) {
        return this->unbuffered_read_into(destination, size);
    }
    CONTRACTS_ASSERT(this->has_read_buffer());

    FileBuffer& readBuffer = this->readBuffer_f.value();

    char* destPtr = static_cast<char*>(destination);
    SizeBytes bytesRead = 0;
    while (bytesRead < size) {
        const SizeBytes bytesLeftToRead = size - bytesRead;
        if (readBuffer.readable_bytes() == 0) {
            const SimpleResult rereadResult = this->reread();
            if (rereadResult.is_failure()) {
                const File::Error error = rereadResult.failure_value_or_panic();
                if (error == File::Error::END_OF_FILE) {
                    return pican::Result<Offset, File::Error>::success_by_copy(bytesRead);
                }
            }
        }
        const SizeBytes readBytesIntoBuffer = readBuffer.read_into(destPtr + bytesRead, bytesLeftToRead);
        bytesRead += readBytesIntoBuffer;
    }

    return pican::Result<Offset, File::Error>::success_by_copy(bytesRead);
}

Result<SizeBytes, File::Error>
File::unbuffered_read_into(mem::Block& destination) const& {
    return this->unbuffered_read_into(destination.address_to_ptr<void>(), destination.size_bytes());
}

Result<SizeBytes, File::Error>
File::unbuffered_read_into(void* destination, SizeBytes size) const& {
    if (!this->isOpen_f) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (destination == nullptr) {
        return pican::Result<SizeBytes, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }
    if (this->has_read_buffer()) {
        this->actual_seek(this->latest_read_offset());
        this->readBuffer_f.value().clear();
    }
    CONTRACTS_ASSERT(this->lastReadOffset_f == this->latest_read_offset());

    const Result<SizeBytes, File::Error> readResult = this->actual_read_into(destination, size);
    if (readResult.is_failure()) {
        return readResult;
    }
    this->lastReadOffset_f += readResult.success_value_or_panic();

    return readResult;
}

Result<SizeBytes, File::Error>
File::total_size_bytes() const& {
    return File::total_size_bytes(this->path_f);
}

FilePath
File::path() const& {
    return this->path_f;
}

Result<FileMode, File::Error>
File::mode() const& {
    if (this->isOpen_f) {
        return Result<FileMode, File::Error>::success_by_copy(this->mode_f);
    }
    return Result<FileMode, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
}

Result<FileType, File::Error>
File::file_type() const& {
    return File::file_type(this->path_f);
}

bool
File::is_open() const& {
    return this->isOpen_f;
}

bool
File::is_seekable() const& {
    return File::is_seekable(this->path_f);
}

FileDescriptor
File::file_descriptor() const& {
    return this->descriptor_f;
}

bool
File::exists() const& {
    return File::exists(this->path_f);
}

File::SimpleResult
File::remove() & {
    this->close();
    return File::remove(this->path_f);
}

File::SimpleResult
File::sync() & {
    if (!this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    this->flush();
    const int res = ::fsync(this->descriptor_f);
    if (res != 0) {
        const int err = errno;
        pican::log_error("Error syncing file path: {} err: {}", this->path_f, ::strerror(err));
        return SimpleResult::failure_by_copy(File::Error::UNKNOWN);
    }
    return SimpleResult::success_default();
}

File::SimpleResult
File::flush() & {
    if (!this->has_write_buffer()) {
        return File::SimpleResult::failure_by_copy(File::Error::NO_BUFFER);
    }
    FileBuffer& buffer = this->writBuffer_f.value();

    // nothing to flush
    if (buffer.readable_bytes() == 0) {
        return File::SimpleResult::success_default();
    }

    const Result<SizeBytes, File::Error> result =
        this->actual_write_from(buffer.block().address_to_ptr<void>(), buffer.readable_bytes());
    if (result.is_failure()) {
        return File::SimpleResult::failure_by_copy(result.failure_value_or_panic());
    }
    const SizeBytes wrote = result.success_value_or_panic();
    this->lastWriteOffset_f += wrote;
    buffer.clear();

    return File::SimpleResult::success_default();
}

File::SimpleResult
File::reread() const& {
    if (!this->has_read_buffer()) {
        return File::SimpleResult::failure_by_copy(File::Error::NO_BUFFER);
    }
    FileBuffer& buffer = this->readBuffer_f.value();

    const Result<SizeBytes, File::Error> seekResult = this->actual_seek(this->latest_read_offset());
    if (seekResult.is_failure()) {
        return File::SimpleResult::failure_by_copy(seekResult.failure_value_or_panic());
    }
    buffer.clear();
    const Result<SizeBytes, File::Error> readResult =
        this->actual_read_into(buffer.block().address_to_ptr<void>(), buffer.writable_bytes());
    if (readResult.is_failure()) {
        return File::SimpleResult::failure_by_copy(readResult.failure_value_or_panic());
    }
    const SizeBytes read = readResult.success_value_or_panic();
    buffer.increment_write_index_by(read);

    return File::SimpleResult::success_default();
}

File::SimpleResult
File::clear() & {
    if (!this->isOpen_f) {
        return File::SimpleResult::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }

    const int res = ::ftruncate(this->descriptor_f, 0);
    if (res != 0) {
        const int err = errno;
        pican::log_error("Error truncating file path: {} err: {}", this->path_f, ::strerror(err));
        return SimpleResult::failure_by_copy(File::Error::UNKNOWN);
    }
    this->lastWriteOffset_f = 0;
    return SimpleResult::success_default();
}

bool
File::can_flush() const& {
    if (!this->has_write_buffer()) {
        return false;
    }
    return this->writBuffer_f.value().readable_bytes() > 0;
}

Result<SizeBytes, File::Error>
File::actual_write_from(void* source, SizeBytes size) & {
    const ssize_t written = ::write(this->descriptor_f, source, size);
    if (written == -1) {
        const int err = errno;
        pican::log_error("Error writing file path: {} err: {}", this->path_f, ::strerror(err));
        switch (err) {
            default: {
                return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::UNKNOWN);
            }
        }
    }

    return pican::Result<Offset, File::Error>::success_by_copy(written);
}

Result<SizeBytes, File::Error>
File::actual_read_into(void* destination, SizeBytes size) const& {
    const ssize_t read = ::read(this->descriptor_f, destination, size);
    if (read == -1) {
        const int err = errno;
        pican::log_error("Error reading file path: {} err: {}", this->path_f, ::strerror(err));
        switch (err) {
            default: {
                return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::UNKNOWN);
            }
        }
    }
    if (read == 0) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::END_OF_FILE);
    }

    return pican::Result<Offset, File::Error>::success_by_copy(read);
}

Result<Offset, File::Error>
File::actual_seek(Offset offset) const& {
    const int res = ::lseek(this->descriptor_f, offset, SEEK_SET);
    if (res == -1) {
        const int err = errno;
        pican::log_error("Error seeking file path: {} offset: {} err: {}", this->path_f, offset, ::strerror(err));
        switch (err) {
            default: {
                return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::UNKNOWN);
            }
        }
    }
    this->lastWriteOffset_f = res;
    this->lastReadOffset_f = res;
    return pican::Result<Offset, File::Error>::success_by_copy(this->lastWriteOffset_f);
}

Offset
File::latest_read_offset() const& {
    if (!this->has_read_buffer()) {
        return this->lastReadOffset_f;
    }
    return this->lastReadOffset_f + this->readBuffer_f.value().read_index();
}

Offset
File::latest_write_offset() const& {
    if (!this->has_write_buffer()) {
        return this->lastWriteOffset_f;
    }
    return this->lastWriteOffset_f + this->writBuffer_f.value().read_index();
}

/* static */
bool
File::exists(FilePath path) {
    return ::access(path.data(), F_OK) == 0;
}

bool
File::is_readable(FilePath path) {
    return ::access(path.data(), R_OK) == 0;
}

bool
File::is_writable(FilePath path) {
    return ::access(path.data(), W_OK) == 0;
}

bool
File::is_seekable(FilePath path) {
    const Result<FileType, File::Error> fileTypeResult = File::file_type(path);
    if (fileTypeResult.is_failure()) {
        return false;
    }

    return fileTypeResult.success_value_or_panic() == FileType::REGULAR_FILE;
}

/* static */
File::SimpleResult
File::remove(FilePath path) {
    const int res = ::unlink(path.data());
    if (res != 0) {
        const int err = errno;
        pican::log_error("Error unlinking file path: {} err: {}", path, ::strerror(err));
        return SimpleResult::failure_by_copy(File::Error::UNKNOWN);
    }
    return SimpleResult::success_default();
}

/* static */
Result<FileType, File::Error>
File::file_type(FilePath path) {
    Stat stats{};
    const int res = ::lstat(path.data(), &stats);
    if (res != 0) {
        const int err = errno;
        pican::log_error("Could not stat file for file path: {}, err: {}", path, ::strerror(err));
        return Result<FileType, File::Error>::failure_by_copy(File::Error::CANNOT_STAT);
    }
    FileType type = FileType::REGULAR_FILE;
    const auto statsMode = stats.st_mode;
    if (S_ISREG(statsMode)) {
        type = FileType::REGULAR_FILE;
    } else if (S_ISDIR(statsMode)) {
        type = FileType::DIRECTORY;
    } else if (S_ISBLK(statsMode)) {
        type = FileType::BLOCK_DEVICE;
    } else if (S_ISFIFO(statsMode)) {
        type = FileType::FIFO_PIPE;
    } else if (S_ISLNK(statsMode)) {
        type = FileType::LINK;
    } else if (S_ISSOCK(statsMode)) {
        type = FileType::SOCKET;
    } else if (S_ISCHR(statsMode)) {
        type = FileType::CHAR_DEVICE;
    } else {
        pican::log_error("Could not determine file type for file path: {}", path);
        return Result<FileType, File::Error>::failure_by_copy(File::Error::CANNOT_STAT);
    }
    return Result<FileType, File::Error>::success_by_copy(type);
}

/* static */
Result<SizeBytes, File::Error>
File::total_size_bytes(FilePath path) {
    Stat stats{};
    const int res = ::stat(path.data(), &stats);
    if (res != 0) {
        const int err = errno;
        pican::log_error("Could not stat file for file path: {}, err: {}", path, ::strerror(err));
        return Result<SizeBytes, File::Error>::failure_by_copy(File::Error::CANNOT_STAT);
    }

    const auto statsMode = stats.st_mode;
    if (!S_ISREG(statsMode)) {
        return Result<SizeBytes, File::Error>::failure_by_copy(File::Error::CANNOT_STAT);
    }
    return Result<SizeBytes, File::Error>::success_by_copy(stats.st_size);
}

}  // namespace pican
