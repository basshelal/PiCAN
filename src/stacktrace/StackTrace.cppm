module;

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include <backtrace.h>
#include <unistd.h>

export module stacktrace;

// TODO @basshelal Wed 26-Aug-2026 : Make the libbacktrace dependency using no system libraries, use cpm + cmake
//  entirely
export namespace stacktrace {
struct EntryString {
    char value[256];
    std::size_t length = 256;
};

struct TraceEntry {
    uintptr_t programCounter;
    EntryString symbolName;
    uintptr_t symbolAddress;
    uintptr_t symbolSize;
    uintptr_t symbolOffset;
    EntryString filename;
    int lineNumber;
    EntryString functionName;
};
}  // namespace stacktrace

namespace {
// Global state required by libbacktrace
struct backtrace_state* libbacktraceState_g = nullptr;

struct CallbackData {
    stacktrace::TraceEntry* entries;
    std::size_t entryCount;
    std::size_t currentIndex;
};

void
print_string(const char* str, int fd) {
    ::write(fd, str, strlen(str));
}

[[maybe_unused]]
// Minimal integer to string converter to avoid sprintf malloc risks
void
print_hex(uintptr_t val, int fd) {
    char buf[32];
    buf[0] = '0';
    buf[1] = 'x';
    ::snprintf(buf, sizeof(buf), "0x%lx", val);
    print_string(buf, fd);
}

[[maybe_unused]]
void
print_int(int val, int fd) {
    char buf[16];
    ::snprintf(buf, sizeof(buf), "%d", val);
    print_string(buf, fd);
}

int
empty_callback(void* data, uintptr_t programCounter, const char* fileName, int lineNumber, const char* functionName) {
    return 0;
}

// Called if libbacktrace has an internal error
void
error_callback(void* data, const char* msg, int errnum) {
    print_string("[Backtrace Error]: ", STDERR_FILENO);
    print_string(msg, STDERR_FILENO);
    print_string("\n", STDERR_FILENO);
}

void
syminfo_callback(
    void* data, uintptr_t programCounter, const char* symbolName, uintptr_t symbolAddress, uintptr_t symbolSize
) {
    stacktrace::TraceEntry* entry = static_cast<stacktrace::TraceEntry*>(data);

    entry->programCounter = programCounter;
    if (symbolName != nullptr) {
        const std::size_t length = std::min(entry->symbolName.length, std::strlen(symbolName));
        ::memcpy(entry->symbolName.value, symbolName, length);
    }
    entry->symbolAddress = symbolAddress;
    entry->symbolSize = symbolSize;

    if (symbolAddress != 0) {
        const uintptr_t offset = programCounter - symbolAddress;
        entry->symbolOffset = offset;
    }
}

int
full_callback(void* data, uintptr_t programCounter, const char* filename, int lineNumber, const char* functionName) {
    CallbackData* callbackData = static_cast<CallbackData*>(data);
    stacktrace::TraceEntry* entry = callbackData->entries + callbackData->currentIndex;
    if (callbackData->currentIndex >= callbackData->entryCount) {
        return 1;
    }
    ::backtrace_syminfo(libbacktraceState_g, programCounter, syminfo_callback, error_callback, entry);

    // These can and will be mangled but can still be quite readable
    // de-mangling at runtime using abi::__cxa_demangle will use the heap and thus is not a viable option for us
    // there is no simple way to get runtime de-mangling, therefore, we resort to keeping the mangled names,
    // use a tool like c++filt to de-mangle the names from the stacktrace
    if (functionName != nullptr) {
        const std::size_t length = std::min(entry->functionName.length, std::strlen(functionName));
        ::memcpy(entry->functionName.value, functionName, length);
    }

    if (filename != nullptr) {
        const std::size_t length = std::min(entry->filename.length, std::strlen(filename));
        ::memcpy(entry->filename.value, filename, length);
    }
    entry->lineNumber = lineNumber;
    ++callbackData->currentIndex;
    return 0;
}

static_assert(std::is_same_v<decltype(&full_callback), ::backtrace_full_callback>);

constexpr std::size_t ENTRY_COUNT = 32;
stacktrace::TraceEntry entries_g[ENTRY_COUNT];
}  // namespace

export namespace stacktrace {

void
initialize(char** argv) {
    if (libbacktraceState_g != nullptr) {
        return;
    }
    char* programPath = nullptr;
    if (argv != nullptr) {
        programPath = argv[0];
    }
    // allocates memory using mmap so it is safe even when the heap (new/malloc) is disabled
    libbacktraceState_g = ::backtrace_create_state(programPath, 1, error_callback, nullptr);
    // a dummy call that forces libbacktrace to load its internals (mmaps, locks, allocs etc)
    // and make it safe to use from within a signal
    ::backtrace_full(libbacktraceState_g, 0, empty_callback, error_callback, nullptr);
}

// TODO @basshelal Thu 27-Aug-2026 : Still not finished the design of this!
std::size_t
get_stacktrace(TraceEntry* entries, std::size_t entry_count, int skipFrames = 0) {
    CallbackData callbackData{.entries = entries, .entryCount = entry_count, .currentIndex = 0};

    if (libbacktraceState_g == nullptr) {
        print_string("Backtrace state not initialized!\n", STDERR_FILENO);
        return 0;
    }
    // backtrace_full uses mmap for temporary storage, safe for disabled heap
    // always skip +1 frames to remove this function call (just noise)
    ::backtrace_full(libbacktraceState_g, skipFrames + 1, full_callback, error_callback, &callbackData);
    return callbackData.currentIndex;
}

void
print_stacktrace(std::FILE* file, int skipFrames = 0) {
    // const int fd = ::fileno(file);
    const std::size_t entryCount = stacktrace::get_stacktrace(entries_g, 32, 1);
    for (std::size_t i = 0; i < entryCount; ++i) {
        const TraceEntry& entry = entries_g[i];
        printf(
            "file: %s, line: %d, function: %s, symbol: %s, address: %lu, offset: %lu, size: %lu, pc: %lu\n",
            entry.filename.value, entry.lineNumber, entry.functionName.value, entry.symbolName.value,
            entry.symbolAddress, entry.symbolOffset, entry.symbolSize, entry.programCounter
        );
    }
}
}  // namespace stacktrace
