#pragma once

#include <cstdint>
#include <optional>

#include "pican/FileBuffer.hpp"
#include "pican/Result.hpp"
#include "pican/mem/Block.hpp"

namespace pican {

constexpr FileDescriptor NULL_FILE_DESCRIPTOR = -1;

enum class FileMode : std::uint8_t {
    READ_ONLY,
    WRITE_ONLY,
};

// TODO @basshelal Tue 24-Feb-2026 : How will we deal with directories????
enum class FileType : std::uint8_t {
    REGULAR_FILE,
    DIRECTORY,
    BLOCK_DEVICE,
    FIFO_PIPE,
    LINK,
    SOCKET,
    CHAR_DEVICE,
};

class File {
public:  // types
    enum class Error : std::uint8_t {
        UNKNOWN,
        FILE_NOT_FOUND,
        FILE_IS_DIR,
        PERMISSION_DENIED,
        READ_ONLY_FILESYSTEM,
        FILE_NOT_OPEN,
        INCORRECT_MODE,
        NULL_BUFFER,
        FILE_OPEN,
        CANNOT_STAT,
        NO_BUFFER,
        NOT_SEEKABLE,
        END_OF_FILE
    };

    using SimpleResult = pican::SimpleResult<Error>;

private:  // fields
    FilePath path_f;
    FileDescriptor descriptor_f;
    FileMode mode_f;
    bool isOpen_f;
    mutable std::optional<FileBuffer> readBuffer_f;
    mutable Offset lastReadOffset_f;
    mutable std::optional<FileBuffer> writBuffer_f;
    mutable Offset lastWriteOffset_f;

public:  // constructor
    explicit File(FilePath path);

public:  // lifetime
    File(const File& rhs) = delete;

    File(File&& rhs) noexcept;

    File&
    operator=(const File& rhs) & = delete;

    File&
    operator=(File&& rhs) & noexcept;

    ~File();

public:  // member functions
    SimpleResult
    set_read_buffer(const mem::Block& block) &;

    SimpleResult
    remove_read_buffer() &;

    [[nodiscard]]
    bool
    has_read_buffer() const&;

    SimpleResult
    set_write_buffer(const mem::Block& block) &;

    SimpleResult
    remove_write_buffer() &;

    [[nodiscard]]
    bool
    has_write_buffer() const&;

    SimpleResult
    open(FileMode mode, bool create) &;

    SimpleResult
    close() &;

    Result<Offset, Error>
    seek_to(Offset offset) &;

    Result<SizeBytes, Error>
    write_from(const mem::Block& source) &;

    Result<SizeBytes, Error>
    write_from(void* source, SizeBytes size) &;

    Result<SizeBytes, Error>
    unbuffered_write_from(const mem::Block& source) &;

    Result<SizeBytes, Error>
    unbuffered_write_from(void* source, SizeBytes size) &;

    Result<SizeBytes, Error>
    read_into(mem::Block& destination) const&;

    Result<SizeBytes, Error>
    read_into(void* destination, SizeBytes size) const&;

    Result<SizeBytes, Error>
    unbuffered_read_into(mem::Block& destination) const&;

    Result<SizeBytes, Error>
    unbuffered_read_into(void* destination, SizeBytes size) const&;

    [[nodiscard]]
    Result<SizeBytes, File::Error>
    total_size_bytes() const&;

    [[nodiscard]]
    FilePath
    path() const&;

    [[nodiscard]]
    Result<FileMode, File::Error>
    mode() const&;

    [[nodiscard]]
    Result<FileType, File::Error>
    file_type() const&;

    [[nodiscard]]
    bool
    is_open() const&;

    [[nodiscard]]
    bool
    is_seekable() const&;

    [[nodiscard]]
    FileDescriptor
    file_descriptor() const&;

    [[nodiscard]]
    bool
    exists() const&;

    SimpleResult
    remove() &;

    SimpleResult
    sync() &;

    SimpleResult
    flush() &;

    SimpleResult
    reread() const&;

    SimpleResult
    clear() &;

    [[nodiscard]]
    bool
    can_flush() const&;

private:  // member functions
    Result<SizeBytes, File::Error>
    actual_write_from(void* source, SizeBytes size) &;

    Result<SizeBytes, File::Error>
    actual_read_into(void* destination, SizeBytes size) const&;

    Result<Offset, File::Error>
    actual_seek(Offset offset) const&;

    [[nodiscard]]
    Offset
    latest_read_offset() const&;

    [[nodiscard]]
    Offset
    latest_write_offset() const&;

public:  // static functions
    [[nodiscard]]
    static bool
    exists(FilePath path);

    [[nodiscard]]
    static bool
    is_readable(FilePath path);

    [[nodiscard]]
    static bool
    is_writable(FilePath path);

    [[nodiscard]]
    static bool
    is_seekable(FilePath path);

    static SimpleResult
    remove(FilePath path);

    [[nodiscard]]
    static Result<FileType, File::Error>
    file_type(FilePath path);

    [[nodiscard]]
    static Result<SizeBytes, File::Error>
    total_size_bytes(FilePath path);
};

}  // namespace pican
