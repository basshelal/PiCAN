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
    path_f{path}, descriptor_f{NULL_FILE_DESCRIPTOR}, mode_f{FileMode::READ_ONLY}, isOpen_f{false},
    type_f{FileType::REGULAR_FILE}, lastCommitedOffset_f{0}, readBuffer_f{}, writBuffer_f{}, readBufferOffset_f{0},
    writeBufferOffset_f{0} {
}

File::File(File&& rhs) noexcept :
    path_f{rhs.path_f}, descriptor_f{rhs.descriptor_f}, mode_f{rhs.mode_f}, isOpen_f{rhs.isOpen_f}, type_f{rhs.type_f},
    lastCommitedOffset_f{rhs.lastCommitedOffset_f}, readBuffer_f{std::move(rhs.readBuffer_f)},
    writBuffer_f{std::move(rhs.writBuffer_f)}, readBufferOffset_f{rhs.readBufferOffset_f},
    writeBufferOffset_f{rhs.writeBufferOffset_f} {
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
    this->type_f = rhs.type_f;
    this->lastCommitedOffset_f = rhs.lastCommitedOffset_f;
    this->readBuffer_f = std::move(rhs.readBuffer_f);
    this->writBuffer_f = std::move(rhs.writBuffer_f);
    this->readBufferOffset_f = rhs.readBufferOffset_f;
    this->writeBufferOffset_f = rhs.writeBufferOffset_f;

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
    this->readBuffer_f.emplace(block, FileBuffer::Type::READ);
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
    this->writBuffer_f.emplace(block, FileBuffer::Type::WRITE);
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
File::open(FileMode mode, bool create, bool append) & {
    const std::string_view modeString = magic_enum::enum_name<FileMode>(this->mode_f);
    int flags = 0;
    mode_t filePermissions = 0666;  // read/write permissions, used only if the file is created
    switch (mode) {
        case FileMode::READ_ONLY: {
            flags = O_RDONLY;
            break;
        }
        case FileMode::WRITE_ONLY: {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        }
        case FileMode::READ_WRITE: {
            flags = O_RDWR | O_CREAT;
            break;
        }
    }
    if (create) {
        flags |= O_CREAT;
    }
    if (append) {
        flags |= O_APPEND;
    }

    FileDescriptor fd = ::open(this->path_f.data(), flags, filePermissions);
    if (fd == NULL_FILE_DESCRIPTOR) {
        const int err = errno;
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

    const Result<FileType, File::Error> fileTypeResult = File::file_type(this->path_f);

    if (fileTypeResult.is_failure()) {
        return File::SimpleResult::failure_by_copy(File::Error::CANNOT_STAT);
    }
    this->type_f = fileTypeResult.success_value_or_panic();

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

    return File::SimpleResult::success_default();
}

Offset
File::current_offset() const& {
    if (!this->isOpen_f) {
        return 0;
    }
    return this->lastCommitedOffset_f;
}

// TODO @basshelal Wed 25-Feb-2026 : Note! Do something about this and test it!
//  If the O_APPEND file status flag is set on the open file
//  description, then a write(2) always moves the file offset to the
//  end of the file, regardless of the use of lseek().

Result<Offset, File::Error>
File::seek_to(Offset offset) & {
    if (!this->is_seekable()) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::NOT_SEEKABLE);
    }
    if (!this->isOpen_f) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (this->has_unflushed_data()) {
        this->flush();
    }

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
    this->lastCommitedOffset_f = res;
    return pican::Result<Offset, File::Error>::success_by_copy(this->lastCommitedOffset_f);
}

Result<SizeBytes, File::Error>
File::write_from(const mem::Block& source) & {
    return this->write_from(source.address_to_ptr<void>(), source.size_bytes());
}

Result<SizeBytes, File::Error>
File::write_from(void* source, SizeBytes size) & {
    if (!this->isOpen_f) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (source == nullptr) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }
    if (!this->has_write_buffer()) {
        return this->unbuffered_write_from(source, size);
    }
    CONTRACTS_ASSERT(this->has_write_buffer());

    FileBuffer& writeBuffer = this->writBuffer_f.value();

    // optimization: if requesting to write >= what the buffer can handle, just do 1 direct write and clear the buffer
    if (size >= writeBuffer.remaining_bytes()) {
        // need to flush any uncommitted changes first, does nothing if there are none
        this->flush();
        const Result<SizeBytes, File::Error> writeResult = this->actual_write(source, size);
        if (writeResult.is_success()) {
            this->lastCommitedOffset_f += writeResult.success_value_or_panic();
        }
        return writeResult;
    }

    CONTRACTS_ASSERT(writeBuffer.remaining_bytes() > size);
    const SizeBytes wroteBytes = writeBuffer.write_from(source, size);
    CONTRACTS_ASSERT(wroteBytes == size);
    CONTRACTS_ASSERT(this->has_unflushed_data());

    return pican::Result<Offset, File::Error>::success_by_copy(wroteBytes);
}

Result<SizeBytes, File::Error>
File::unbuffered_write_from(const mem::Block& source) & {
    return this->unbuffered_write_from(source.address_to_ptr<void>(), source.size_bytes());
}

Result<SizeBytes, File::Error>
File::unbuffered_write_from(void* source, SizeBytes size) & {
    if (!this->isOpen_f) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (source == nullptr) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }

    // need to flush any uncommitted changes first, does nothing if there are none
    if (this->has_write_buffer()) {
        this->flush();
    }

    const Result<SizeBytes, File::Error> writeResult = this->actual_write(source, size);
    if (writeResult.is_success()) {
        this->lastCommitedOffset_f += writeResult.success_value_or_panic();
    }

    return writeResult;
}

Result<SizeBytes, File::Error>
File::read_into(void* destination, SizeBytes size) const& {
    if (!this->isOpen_f) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::FILE_NOT_OPEN);
    }
    if (destination == nullptr) {
        return pican::Result<Offset, File::Error>::failure_by_copy(File::Error::NULL_BUFFER);
    }

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
    this->lastCommitedOffset_f += read;

    return pican::Result<Offset, File::Error>::success_by_copy(read);
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
    if (this->isOpen_f) {
        return Result<FileType, File::Error>::success_by_copy(this->type_f);
    }
    return File::file_type(this->path_f);
}

bool
File::is_open() const& {
    return this->isOpen_f;
}

bool
File::is_seekable() const& {
    const Result<FileType, File::Error> fileTypeResult = this->file_type();
    if (fileTypeResult.is_failure()) {
        return false;
    }

    return fileTypeResult.success_value_or_panic() == FileType::REGULAR_FILE;
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

bool
File::has_unflushed_data() & {
    if (!this->has_write_buffer()) {
        return false;
    }
    FileBuffer& buffer = this->writBuffer_f.value();
    return buffer.offset() > 0;
}

File::SimpleResult
File::flush() & {
    if (!this->has_write_buffer()) {
        return File::SimpleResult::failure_by_copy(File::Error::NO_BUFFER);
    }
    FileBuffer& buffer = this->writBuffer_f.value();

    // nothing to flush
    if (buffer.offset() == 0) {
        return File::SimpleResult::success_default();
    }

    const Result<SizeBytes, File::Error> result =
        this->actual_write(buffer.block().address_to_ptr<void>(), buffer.offset());
    if (result.is_failure()) {
        return File::SimpleResult::failure_by_copy(result.failure_value_or_panic());
    }
    this->lastCommitedOffset_f += result.success_value_or_panic();
    buffer.set_offset(0);

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
    this->lastCommitedOffset_f = 0;
    return SimpleResult::success_default();
}

/* static */
bool
File::exists(FilePath path) {
    return ::access(path.data(), F_OK) == 0;
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

Result<SizeBytes, File::Error>
File::actual_write(void* source, SizeBytes size) & {
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

}  // namespace pican
