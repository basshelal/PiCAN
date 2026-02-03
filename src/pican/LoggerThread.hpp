#pragma once

#include "Logger.hpp"
#include "pican/Array.hpp"
#include "pican/Thread.hpp"

namespace pican {
class LoggerThread {
private:  // fields
    static Thread* thread_f;

private:  // types
    using This = LoggerThread;

private:  // constructor
    LoggerThread() {
    }

public:  // member functions
    static void
    initialize();

    static void
    start();

private:  // member functions
    static void
    ensure_initialized();
};
}  // namespace pican
