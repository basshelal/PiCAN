module;

#include <algorithm>
#include <array>
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

struct Entry {
    static constexpr std::size_t SYMBOL_FUNCTION_LENGTH = 128;
    static constexpr std::size_t FILE_LENGTH = 256;
    uintptr_t programCounter = 0;
    uintptr_t symbolAddress = 0;
    uintptr_t symbolSize = 0;
    uintptr_t symbolOffset = 0;
    int lineNumber = 0;
    // TODO(bxh) 02-Sep-26 22:59 Consider converting to std::array??
    char symbolName[SYMBOL_FUNCTION_LENGTH] = {};
    char functionName[SYMBOL_FUNCTION_LENGTH] = {};
    char filename[FILE_LENGTH] = {};
};

using EntryCallback = bool (*)(const stacktrace::Entry& entry, std::size_t index, void* userData);
}  // namespace stacktrace

namespace {
// Global state required by libbacktrace
struct backtrace_state* libbacktraceState_g = nullptr;

class FdWriter {
private:
    int fd_m;

public:
    explicit FdWriter(int fd) : fd_m(fd) {
    }

    void
    write_string(const char* string) const {
        ::write(this->fd_m, string, strlen(string));
    }

    void
    write_hex(uintptr_t val) const {
        char buf[32];
        buf[0] = '0';
        buf[1] = 'x';
        ::snprintf(buf, sizeof(buf), "0x%lx", val);
        this->write_string(buf);
    }

    void
    write_int(int val) const {
        char buf[16];
        ::snprintf(buf, sizeof(buf), "%d", val);
        this->write_string(buf);
    }
};

int
empty_callback(void* data, uintptr_t programCounter, const char* fileName, int lineNumber, const char* functionName) {
    return 0;
}

// Called if libbacktrace has an internal error
void
error_callback(void* data, const char* msg, int errnum) {
    const FdWriter writer{STDERR_FILENO};
    writer.write_string("[Backtrace Error]: ");
    writer.write_string(msg);
    writer.write_string("\n");
}

static_assert(std::is_same_v<decltype(&error_callback), ::backtrace_error_callback>);

struct CallbackInfo {
    stacktrace::Entry entry = {};
    stacktrace::EntryCallback callback = {};
    std::size_t index = 0;
    void* userData = nullptr;
};

void
syminfo_callback(
    void* data, uintptr_t programCounter, const char* symbolName, uintptr_t symbolAddress, uintptr_t symbolSize
) {
    CallbackInfo* info = static_cast<CallbackInfo*>(data);
    stacktrace::Entry& entry = info->entry;
    entry.programCounter = programCounter;
    if (symbolName != nullptr) {
        const std::size_t length = std::min(stacktrace::Entry::SYMBOL_FUNCTION_LENGTH, std::strlen(symbolName));
        ::memcpy(entry.symbolName, symbolName, length);
    }
    entry.symbolAddress = symbolAddress;
    entry.symbolSize = symbolSize;

    if (symbolAddress != 0) {
        const uintptr_t offset = programCounter - symbolAddress;
        entry.symbolOffset = offset;
    }
}

static_assert(std::is_same_v<decltype(&syminfo_callback), ::backtrace_syminfo_callback>);

int
full_callback(void* data, uintptr_t programCounter, const char* filename, int lineNumber, const char* functionName) {
    CallbackInfo* info = static_cast<CallbackInfo*>(data);
    info->entry = stacktrace::Entry{};
    stacktrace::Entry& entry = info->entry;
    stacktrace::EntryCallback callback = info->callback;
    // These can and will be mangled but can still be quite readable
    // de-mangling at runtime using abi::__cxa_demangle will use the heap and thus is not a viable option for us
    // there is no simple way to get runtime de-mangling, therefore, we resort to keeping the mangled names,
    // use a tool like c++filt to de-mangle the names from the stacktrace
    if (functionName != nullptr) {
        const std::size_t length = std::min(stacktrace::Entry::SYMBOL_FUNCTION_LENGTH, std::strlen(functionName));
        ::memcpy(entry.functionName, functionName, length);
    }

    if (filename != nullptr) {
        const std::size_t length = std::min(stacktrace::Entry::FILE_LENGTH, std::strlen(filename));
        ::memcpy(entry.filename, filename, length);
    }
    entry.lineNumber = lineNumber;

    const ::backtrace_syminfo_callback syminfoCallback = [](void* data, uintptr_t programCounter,
                                                            const char* symbolName, uintptr_t symbolAddress,
                                                            uintptr_t symbolSize) -> void {
        CallbackInfo* info = static_cast<CallbackInfo*>(data);
        stacktrace::Entry& entry = info->entry;
        entry.programCounter = programCounter;
        if (symbolName != nullptr) {
            const std::size_t length = std::min(stacktrace::Entry::SYMBOL_FUNCTION_LENGTH, std::strlen(symbolName));
            ::memcpy(entry.symbolName, symbolName, length);
        }
        entry.symbolAddress = symbolAddress;
        entry.symbolSize = symbolSize;

        if (symbolAddress != 0) {
            const uintptr_t offset = programCounter - symbolAddress;
            entry.symbolOffset = offset;
        }
    };

    ::backtrace_syminfo(libbacktraceState_g, programCounter, syminfoCallback, error_callback, data);
    const bool keepGoing = callback(entry, info->index, info->userData);
    if (!keepGoing) {
        return 1;
    }
    info->index++;
    return 0;
}

static_assert(std::is_same_v<decltype(&full_callback), ::backtrace_full_callback>);

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

void
get_stacktrace(EntryCallback callback, void* userData = nullptr, int skipFrames = 0) {
    if (callback == nullptr) {
        return;
    }
    if (libbacktraceState_g == nullptr) {
        const FdWriter writer{STDERR_FILENO};
        writer.write_string("Backtrace state not initialized!\n");
        return;
    }
    CallbackInfo info{.entry = Entry{}, .callback = callback, .index = 0, .userData = userData};
    ::backtrace_full(libbacktraceState_g, skipFrames + 1, full_callback, error_callback, &info);
}

Entry
get_current_entry() {
    EntryCallback callback = [](const Entry& entry, std::size_t index, void* userData) -> bool {
        Entry* resultPtr = static_cast<Entry*>(userData);
        *resultPtr = entry;
        return false;
    };
    Entry result{};
    get_stacktrace(callback, &result, 1);
    return result;
}

std::size_t
get_entries(Entry* entries, std::size_t entryCount, int skipFrames) {
    struct Data {
        Entry* entries;
        std::size_t entryCount;
        std::size_t wrote;
    };

    Data data{.entries = entries, .entryCount = entryCount, .wrote = 0};
    EntryCallback callback = [](const Entry& entry, std::size_t index, void* userData) -> bool {
        Data* data = static_cast<Data*>(userData);
        data->entries[index] = entry;
        data->wrote++;
        return (index <= data->entryCount);
    };
    stacktrace::get_stacktrace(callback, &data, skipFrames + 1);
    return data.wrote;
}

void
print_entry(const Entry& entry, std::FILE* file) {
    const FdWriter writer{::fileno(file)};
    writer.write_string(entry.filename);
    writer.write_string(":");
    writer.write_int(entry.lineNumber);
    writer.write_string(" @ ");
    writer.write_string(entry.functionName);
    writer.write_string(" ");
    writer.write_string(entry.symbolName);
    writer.write_string(" @ ");
    writer.write_hex(entry.symbolAddress);
    writer.write_string(" + ");
    writer.write_hex(entry.symbolOffset);
    writer.write_string(" = ");
    writer.write_hex(entry.programCounter);
    writer.write_string("\n");
}

void
print_stacktrace(std::FILE* file = stderr, int skipFrames = 0) {
    EntryCallback callback = [](const Entry& entry, std::size_t index, void* userData) -> bool {
        std::FILE* file = static_cast<FILE*>(userData);
        print_entry(entry, file);
        return true;
    };
    stacktrace::get_stacktrace(callback, file, skipFrames + 1);
}
}  // namespace stacktrace
