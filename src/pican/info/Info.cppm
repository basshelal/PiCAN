module;

#include <cstdint>

export module pican.info:Info;

import pican.core;

export namespace pican::info {

using Percentage = float;
using Hertz = std::uint32_t;
using MegaHertz = float;
using Celsius = std::int16_t;

struct CpuCore {
    Index index;
    Percentage usage;
    Hertz frequency;
    Celsius temp;
};

struct CpuTotal {
    Percentage usage;
    Hertz frequency;
    Celsius temp;
    Count processes;
    Count processesRunning;
    Count processesBlocked;
};

struct Memory {
    SizeBytes total;
    SizeBytes used;
    SizeBytes free;
    // maybe more from /proc/meminfo
};

struct Process {
    Percentage cpuUsage;
    SizeBytes memoryUsed;
    std::uint64_t uptime;
    // more in /proc/self/ or /proc/<pid>
};

struct Clock {
    std::uint64_t unixClock;
    std::uint64_t systemUptime;
};

struct Network {
    float uploadRate;
    float downloadRate;
    SizeBytes totalDownloaded;
    SizeBytes totalUploaded;
};

struct Disk {
    float readRate;
    float writeRate;
    SizeBytes total;
    SizeBytes used;
    SizeBytes free;
};

struct Power {
    // TODO @basshelal Thu 22-Jan-2026 : Can we get information about system power?
    //  No, we need a separate hardware unit, INA219 or INA226 for higher precision
};

// TODO @basshelal Thu 22-Jan-2026 : Find out more things to add like:
//  iowait, is_throttled, CAN bus health and rate and dropped packets/frames, 4g/LTE signal strength, GPS status,
//  context switches, interrupts

}  // namespace pican::info
