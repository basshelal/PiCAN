#include "pican/log/Sink.hpp"

namespace pican::log {

Sink::Sink(const std::string_view& name, Level level, File&& file) :
    name_f{name}, level_f{level}, file_f{std::move(file)} {
}

const std::string_view&
Sink::name() const& {
    return this->name_f;
}

const Level&
Sink::level() const& {
    return this->level_f;
}

const File&
Sink::file() const& {
    return this->file_f;
}

pican::Result<Sink, Sink::Error>
Sink::create(std::string_view name, Level level, FilePath path) {
    if (!File::exists(path)) {
        return pican::Result<Sink, Sink::Error>::failure_by_copy(Sink::Error::FILE_NOT_FOUND);
    }
    File file{path};
    const File::SimpleResult openResult = file.open(FileMode::WRITE_ONLY, true, true);
    if (openResult.is_failure()) {
        return pican::Result<Sink, Sink::Error>::failure_by_copy(Sink::Error::CANNOT_OPEN_FILE);
    }

    return pican::Result<Sink, Sink::Error>::success_by_move(Sink{name, level, std::move(file)});
}

}  // namespace pican::log
