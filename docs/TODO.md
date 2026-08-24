# TODO

## Done

## Doing

* Comment out all code so that we can have tests running for absolute basic units and modules, tests are black-box only!
  * Testing utils and organize structure of tests to something that makes sense
  * noheap and stacktrace
  * core
  * mem
  * ds
  * fs
  * ...

* IMPORTANT! FIX all in place data holders (like Result and RingBuffer and Array) to ensure
  that copy assignment operators don't happen on uninitialized garbage data, use placement new and remember 
  destructor logic too
* Continue testing Array and Map, possibly removing ArrayList
* CanThread rate limiting (250ms maybe)
* CanThread delta encoding, ignore frames that have not changed since last check
* info thread for reading and capturing system and process info
* Initialization flags through command line flag parsing and environment variables
* Filter CAN frames to what we can decode/parse and are interested in
* Create a simple decoder/parser that is for now hardcoded but allows for the possibility of
  using a DBC file or something like that, we will parse into can::Events

* Create a MemoryManager of some kind for the tests, they will need to have usable memory
* Allow GoogleTest to be whitelisted completely by doing a check in each heap allocation to allow for any allocations
  when the stacktrace contains a location with the string "googletest" or "googletest-src", for this we need to 
  add more functionality to `stacktrace`
* Complete re-check:
  * Tests
  * More cpp over hpp files
  * Contracts as much as possible they should use our stacktrace instead of SourceLocation

## Will Do

* Switch to Catch2
* Consider a CI system or something like that maybe?
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
* Look into sanitizers and valgrind and other tools like that
* Yocto docker, qemu stuff which then needs better scripts and CMake targets and presets and builds
* Pi needs a watchdog to watch that PiCAN is running and if not logs and sends this to the network and re-starts it
* Consider OTA update technology, but not important for now