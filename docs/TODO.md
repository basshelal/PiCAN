# TODO

## Done

* ~~refactor namespaces and directories use `pican` as the prefix:~~
  * ~~`memory`, `ui`, `can`, `sysinfo`, other stuff doesn't _need_ a nested namespace if it doesn't fit in one~~
* ~~Remove the separation of `ui` into a separate directory, we can always do that again later, keep things simple now~~
* ~~wrap malloc and friends to avoid ANY heap allocation, which should just abort/terminate and print the stack trace
  we need to make this work for GoogleTests as well by using `noheap` as a library that disables the heap which is
  added for the main `pican-app`but not for `pican-test` which will have its own heap which will allow GoogleTest
  to use it, then have an RAII scope guarded class to check if any of OUR heap allocations were done using a global
  thread local variable~~
* ~~install and setup libbacktrace so that we can get the backtraces when needed, usually when we abort/terminate~~
* ~~Finish MemoryManager to return and track Memory containers~~
  * ~~Block~~
  * ~~Arena~~
  * ~~Array~~
  * ~~Pool~~
* ~~RingBuffer (try to have 1 implementation for both behaviors, overwriting is very important)~~
* ~~We need a logging system which can log to stdout and any files, must be heap-less and allow for varargs but safe~~
* ~~Custom Thread class wrapping pthreads, need to rethink how we will do inter-thread communication like
  EventNotifier~~
* ~~Git commit and push~~

## Doing

* LoggerThread
* ThreadManager

* Create a MemoryManager of some kind for the tests, they will need to have usable memory
* Allow GoogleTest to be whitelisted completely by doing a check in each heap allocation to allow for any allocations
  when the stacktrace contains a location with the string "googletest" or "googletest-src", for this we need to 
  add more functionality to `stacktrace`
* Complete re-check:
  * Tests
  * More cpp over hpp files
  * Contracts as much as possible they should use our stacktrace instead of SourceLocation

## Will Do

* Pre-allocate and start all Threads, should have 1 UI thread, 1 CAN Thread (reading, processing etc.),
  1 or 2 IO Threads for 4G and disk writing/logging
  * CAN Thread: Reads, filters, processes CAN frames to push into a CANBuffer (map of CAN ID -> RingBuffer)
  * SystemMonitor Thread ? : Reads System stats like CPU usage, memory usage etc and push to a Buffer
  * UI Thread: Reads from CANBuffer and SystemMonitor Info and displays on a UI
  * Logger Thread: Prints logs to stdout and files
  * Network Thread: Pushes packets or messages over the network, can be merged into Disk Thread
  * GPS Thread?
  * Definitely need CAN and UI Threads to be separate, the others can maybe be merged into 1 Thread if necessary
* Look into sanitizers and valgrind and other tools like that
* Yocto docker, qemu stuff which then needs better scripts and CMake targets and presets and builds
* Pi needs a watchdog to watch that PiCAN is running and if not logs and sends this to the network and re-starts it
* Consider OTA update technology, but not important for now