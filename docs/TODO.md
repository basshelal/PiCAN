# TODO

## Done

* Fix up LoggerThread, possibly renaming to Logger, make logging API cleaner
* ThreadManager, make its API also cleaner and easier

## Doing

* pican::can::Thread and friends to read, filter, process and push frames to a buffer, we can
  decide if this thread will be responsible for transforming into app usable messages instead
  of compact proprietary frames, thus we would need a Frame parsing mechanism (hard coded for now is best)

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