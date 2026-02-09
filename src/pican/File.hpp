#pragma once

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "pican/mem/Block.hpp"

namespace pican {

class File {
public:  // types
    // we may change this at a later date to std::filesystem::path
    using Path = std::string_view;
    enum class Mode : std::uint8_t {
        READ,
        WRITE,
        APPEND,
        READ_EXT,
        WRITE_EXT,
        APPEND_EXT,
    };

private:  // fields
    Path path_f;
    Mode mode_f;
    bool isOpen_f;
    mem::Block buffer_f;
    FILE* file_f;

public:  // constructor
    File(Path path, Mode mode, mem::Block buffer) :
        path_f{std::move(path)}, mode_f{std::move(mode)}, isOpen_f{false}, buffer_f{buffer}, file_f{nullptr} {
    }

public:  // copy-control
    File(const File& rhs) = default;

    File(File&& rhs) noexcept = default;

    File&
    operator=(const File& rhs) = default;

    File&
    operator=(File&& rhs) noexcept = default;

    ~File() = default;

public:  // member functions
    void
    open() &;

    void
    close() &;

    void
    write(void* data, SizeBytes elementSize, SizeBytes bufferSize) &;

public:  // getters
    [[nodiscard]]
    Path
    path() const& {
        return this->path_f;
    }

    [[nodiscard]]
    Mode
    mode() const& {
        return this->mode_f;
    }

    [[nodiscard]]
    bool
    is_open() const& {
        return this->isOpen_f;
    }

    [[nodiscard]]
    mem::Block
    buffer() const& {
        return this->buffer_f;
    }
};

}  // namespace pican
