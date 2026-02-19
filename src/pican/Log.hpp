#pragma once

#include <fmt/format.h>
#include <functional>

#include "pican/log/Entry.hpp"
#include "pican/log/Utils.hpp"

namespace pican {

using LogFunctionType = std::function<void(const log::Entry& entry)>;

extern LogFunctionType log_function_g;

template<typename... Args_TP>
inline void
log_error(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    log::Entry entry = pican::log::format_to_entry(pican::log::Level::ERROR, format, std::forward<Args_TP>(args)...);
    pican::log_function_g(entry);
}

template<typename... Args_TP>
inline void
log_warn(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    log::Entry entry = pican::log::format_to_entry(pican::log::Level::WARN, format, std::forward<Args_TP>(args)...);
    pican::log_function_g(entry);
}

template<typename... Args_TP>
inline void
log_info(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    log::Entry entry = pican::log::format_to_entry(pican::log::Level::INFO, format, std::forward<Args_TP>(args)...);
    pican::log_function_g(entry);
}

template<typename... Args_TP>
inline void
log_verbose(fmt::format_string<Args_TP...> format, Args_TP&&... args) {
    log::Entry entry = pican::log::format_to_entry(pican::log::Level::VERBOSE, format, std::forward<Args_TP>(args)...);
    pican::log_function_g(entry);
}
}  // namespace pican
