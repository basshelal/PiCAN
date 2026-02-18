#include <csignal>

#include <magic_enum/magic_enum.hpp>
#include <unistd.h>

#include "noheap/NoHeap.hpp"
#include "pican/Application.hpp"
#include "pican/Array.hpp"
#include "pican/log/LoggerThread.hpp"
#include "pican/log/Sink.hpp"
#include "pican/mem/Manager.hpp"
#include "pican/time/DateTime.hpp"
#include "stacktrace/StackTrace.hpp"

void
cleanup_by_signal(int signal);

void
environment_initialize(int argc, char** argv) {
    // stacktrace needs nothing else and needs to be initialized first
    stacktrace::initialize(argv);
    ::signal(SIGSEGV, ::stacktrace::signal_handler);
    ::signal(SIGILL, ::stacktrace::signal_handler);
    ::signal(SIGABRT, ::stacktrace::signal_handler);
    ::signal(SIGBUS, ::stacktrace::signal_handler);

    ::signal(SIGTERM, cleanup_by_signal);
    ::signal(SIGHUP, cleanup_by_signal);
    ::signal(SIGINT, cleanup_by_signal);

    // noheap disables new and malloc, failures will be reported by stacktrace
    noheap::seal_heap();
    assert(noheap::heap_is_sealed());
}

void
initialize(int argc, char** argv) {
    environment_initialize(argc, argv);

    // pican starts here
    pican::mem::Manager::initialize(pican::mem::Manager::DEFAULT_SIZE);
    pican::Application::initialize();

    pican::mem::Manager::seal();

    pican::Application::start();
}

void
loop() {
    pican::Application::loop();
}

void
cleanup() {
    pican::Application::stop();
}

void
cleanup_by_signal(int signal) {
    ::stacktrace::print_stacktrace();
    cleanup();
}

int
main(int argc, char** argv) {
    // TODO @basshelal Tue 10-Feb-2026 : Set up environment, parse flags etc

    initialize(argc, argv);

    loop();

    cleanup();
}
