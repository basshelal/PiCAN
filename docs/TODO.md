# TODO

* Pipe PICAN CMAKE options from CMAKE into the compile options or definitions or whatever
* Make CMake check that all the files/tools are found and maybe even run an install script run

## Verification

* Make CMake check that all the files/tools are found and maybe even run an install script run
* Use CMake presets/targets for all stages of build and verification (they can call python scripts):
    * Build debug app
    * Build debug tests to run with ASan, MSan, TSan, UBSan and Valgrind
    * Build release (with debug info) app, needs debug info so we can have stacktraces
    * Build release (with debug info) tests to run with ASan, MSan, TSan, UBSan and Valgrind
    * clang-format verify
    * clang-tidy verify
    * semgrep verify
    * cppcheck verify
* Begin adding semgrep rules with help of AI

### Investigate

* Docker build on a CI (very likely GitHub actions)
* Need to have a yocto build on the CI with tests running on the target architecture
* `cppcheck`
* Yocto and qemu emulation
* A code coverage tool
* C Bounded Model Checker

## Tooling

* Convert scripts to python
* Make `init` script to install dependencies like `uv` for simple `git clone` which docker will use

## Documentation

* Document memory locking issue a bit better maybe

## Implementation

* Better way of ignoring unused variables?
* IMPORTANT! Need a better way where we can have `mlockall` _just_ work, no need for sudo and no
  hacks or tricks
* Set panic handler and allow panic to take a fmt formattable string!
* Ensure Result is ready to use, use contracts in it and try to test it as much as possible
* Catch2 needs the heap to be unsealed to write the strings when a test fails
* It also needs exceptions to be available in order to not end with a SIGABRT after the first failed test

* IMPORTANT! FIX all in place data holders (like Result and RingBuffer and Array) to ensure
  that copy assignment operators don't happen on uninitialized garbage data, use placement new and remember
  destructor logic too
* info thread for reading and capturing system and process info
* Initialization flags through command line flag parsing and environment variables
* Filter CAN frames to what we can decode/parse and are interested in
* Create a simple decoder/parser that is for now hardcoded but allows for the possibility of
  using a DBC file or something like that, we will parse into can::Events

## Old Notes

* Pre-allocate and start all Threads, should have 1 UI thread, 1 CAN Thread (reading, processing etc.),
  1 or 2 IO Threads for 4G and disk writing/logging
    * CAN Thread: Reads, filters, processes CAN frames to push into a CANBuffer (map of CAN ID -> RingBuffer)
    * SystemMonitor Thread ? : Reads System stats like CPU usage, memory usage etc and push to a Buffer
    * UI Thread: Reads from CANBuffer and SystemMonitor Info and displays on a UI
    * Logger Thread: Prints logs to stdout and files
    * Network Thread: Pushes packets or messages over the network, can be merged into Disk Thread
    * GPS Thread?
    * Definitely need CAN and UI Threads to be separate, the others can maybe be merged into 1 Thread if necessary
    * Main thread is a watchdog" to ensure that all threads are running and actually doing work (through a heartbeat)
      if a thread is killed or hanging, then we can print the stacktrace and die gracefully to let Linux restart the
      application
* Pi needs a watchdog to watch that PiCAN is running and if not logs and sends this to the network and re-starts it
