#include "pican/File.hpp"

#include <pican/File.hpp>

namespace pican {

namespace {
constexpr const char*
mode_to_string(File::Mode mode) {
    switch (mode) {
        case File::Mode::READ:
            return "r";
        case File::Mode::WRITE:
            return "w";
        case File::Mode::APPEND:
            return "a";
        case File::Mode::READ_EXT:
            return "r+";
        case File::Mode::WRITE_EXT:
            return "w+";
        case File::Mode::APPEND_EXT:
            return "a+";
    }
    pican::panic("Unreachable!");
}
}  // namespace

// TODO @basshelal Thu 05-Feb-2026 : Handle errors somehow!
void
File::open() & {
    FILE* file = ::fopen(this->path_f.data(), mode_to_string(this->mode_f));
    if (file == nullptr) {
        return;
    }
    this->file_f = file;
    this->isOpen_f = true;

    int result = ::setvbuf(this->file_f, this->buffer_f.address_to_ptr<char>(), _IOLBF, this->buffer_f.size_bytes());
    if (result != 0) {
        return;
    }
}

void
File::close() & {
    if (!this->isOpen_f) {
        return;
    }
    ::fclose(this->file_f);
    this->isOpen_f = false;
}

void
File::write(void* data, SizeBytes elementSize, SizeBytes bufferSize) & {
    ::fwrite(data, elementSize, bufferSize, this->file_f);
}

}  // namespace pican
