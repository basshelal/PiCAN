#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

#include "pican/Utils.hpp"

namespace pican::sysinfo {

constexpr const char* MEMINFO_PATH = "/proc/meminfo";
constexpr const char* PROC_SELF_PATH = "/proc/self/status";

[[nodiscard]]
inline std::size_t
read_memory_entry(const char* filePath, const char* entry) {
    const std::size_t entryLength = ::strlen(entry);

    FILE* meminfoFile = ::fopen(filePath, "r");
    if (meminfoFile == nullptr) {
        TODO_NOT_IMPLEMENTED();
    }

    const std::size_t bufferSize = 512;
    char buffer[bufferSize];
    char* lineRead = nullptr;
    bool finished = false;
    std::size_t resultKB = 0;
    do {
        // read file line by line, memory files are line by line
        lineRead = ::fgets(buffer, bufferSize, meminfoFile);

        bool lineFound = ::strncmp(buffer, entry, entryLength) == 0;
        if (lineFound) {
            char* c = lineRead + entryLength;
            while (c != nullptr && !::isdigit(*c)) {
                c++;
            }
            resultKB = ::strtoull(c, nullptr, 10);
        }

    } while (lineRead != nullptr && !finished);

    ::fclose(meminfoFile);

    return resultKB * 1'024;
}

[[nodiscard]]
inline std::size_t
get_total_system_memory() {
    return read_memory_entry(MEMINFO_PATH, "MemTotal");
}

[[nodiscard]]
inline std::size_t
get_free_system_memory() {
    return read_memory_entry(MEMINFO_PATH, "MemAvailable");
}

[[nodiscard]]
inline std::size_t
get_used_system_memory() {
    const std::size_t total = get_total_system_memory();
    const std::size_t free = get_free_system_memory();
    return total - free;
}

[[nodiscard]]
inline std::size_t
get_process_used_memory() {
    return read_memory_entry(PROC_SELF_PATH, "VmRSS");
}

}  // namespace pican::sysinfo
