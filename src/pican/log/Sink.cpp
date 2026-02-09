#include "pican/log/Sink.hpp"

namespace pican::log {

Sink::Sink(const std::string_view& name, Level level, const File& file) : name_f{name}, level_f{level}, file_f{file} {
    // TODO @basshelal Thu 05-Feb-2026 : Convert to Result factory because io could fail
    SANITY_CHECK(file.is_open());
}

Sink::~Sink() {
}
}  // namespace pican::log
