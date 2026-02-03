#include "stacktrace/StackTrace.hpp"

#include <csignal>
#include <cstdio>  // For snprintf (safe if used carefully, or replace with custom itoa)
#include <cstdlib>
#include <cstring>

#include <backtrace.h>
#include <unistd.h>

namespace stacktrace {

namespace {
// Global state required by libbacktrace
struct backtrace_state* libbacktraceState_g = nullptr;

struct CallbackData {
    int fileDescriptor;
};

void
print_string(const char* str, int fd) {
    ::write(fd, str, strlen(str));
}

// Minimal integer to string converter to avoid sprintf malloc risks
void
print_hex(uintptr_t val, int fd) {
    char buf[32];
    buf[0] = '0';
    buf[1] = 'x';
    snprintf(buf, sizeof(buf), "0x%lx", val);
    print_string(buf, fd);
}

void
print_int(int val, int fd) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    print_string(buf, fd);
}

// Called if libbacktrace has an internal error
void
error_callback(void* data, const char* msg, int errnum) {
    CallbackData* callbackData = static_cast<CallbackData*>(data);
    const int fd = callbackData->fileDescriptor;

    print_string("[Backtrace Error]: ", fd);
    print_string(msg, fd);
    print_string("\n", fd);
}

void
syminfo_callback(
    void* data, uintptr_t programCounter, const char* symbolName, uintptr_t symbolAddress, uintptr_t symbolSize
) {
    CallbackData* callbackData = static_cast<CallbackData*>(data);
    const int fd = callbackData->fileDescriptor;

    print_hex(programCounter, fd);

    if (symbolAddress != 0) {
        print_string(" : ", fd);
        print_hex(symbolAddress, fd);
        const uintptr_t offset = programCounter - symbolAddress;
        print_string(" + ", fd);
        print_hex(offset, fd);
    }

    print_string("\n  ", fd);
    if (symbolName != nullptr) {
        print_string(symbolName, fd);
    } else {
        print_string("? symbol unknown ?", fd);
    }
}

int
full_callback(void* data, uintptr_t programCounter, const char* fileName, int lineNumber, const char* functionName) {
    CallbackData* callbackData = static_cast<CallbackData*>(data);
    const int fd = callbackData->fileDescriptor;

    backtrace_syminfo(libbacktraceState_g, programCounter, syminfo_callback, error_callback, data);
    print_string("\n  ", fd);

    if (functionName != nullptr) {
        // These can and will be mangled but can still be quite readable
        print_string(functionName, fd);
    } else {
        print_string("? function unknown ?", fd);
    }

    print_string("\n  ", fd);

    if (fileName != nullptr) {
        print_string(fileName, fd);
        print_string(":", fd);
        print_int(lineNumber, fd);
    } else {
        print_string("? file unknown ?", fd);
    }
    print_string("\n", fd);
    return 0;
}

}  // namespace

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
    libbacktraceState_g = backtrace_create_state(programPath, 1, error_callback, nullptr);
}

void
signal_handler(int signal) {
    int fd = STDERR_FILENO;
    print_string("Signal: ", fd);
    print_string(strsignal(signal), fd);

    print_string("\nStack Trace:\n", fd);
    stacktrace::print_stacktrace();

    _exit(1);  // Immediate exit, do not run destructors
}

void
print_stacktrace(std::FILE* file, int skipFrames) {
    const int fd = ::fileno(file);
    CallbackData callbackData{fd};

    if (libbacktraceState_g != nullptr) {
        // backtrace_full uses mmap for temporary storage, safe for disabled heap
        // always skip +1 frames to remove this function call (just noise)
        backtrace_full(libbacktraceState_g, skipFrames + 1, full_callback, error_callback, &callbackData);
    } else {
        print_string("Backtrace state not initialized!\n", fd);
    }
}
}  // namespace stacktrace
