#pragma once

#include "core/Types.hpp"

namespace pican::sysinfo {
struct SysInfo {
    struct Cpu {
        Float32 usage;             // percent
        Float32 frequency;         // MHz
        Float32 temp;              // C
        Float32 powerConsumption;  // W
        // maybe more load stats from /proc/stat
        // consider stats PER CORE too!
    };

    struct Memory {
        Size total;
        Size used;
        Size free;
        // maybe more from /proc/meminfo
    };

    struct Network {
        Float32 uploadRate;
        Float32 downloadRate;
        Size totalDownloaded;
        Size totalUploaded;
    };

    struct Disk {
        Float32 readRate;
        Float32 writeRate;
        Size total;
        Size used;
        Size free;
    };

    struct Clock {
        UInt64 unixClock;
        UInt64 systemUptime;
    };

    // TODO @basshelal Thu 22-Jan-2026 : For process specific info where do we put it? I think in the relevant struct
    struct Process {
        Float32 cpuUsage;
        Size memoryUsed;
        UInt64 uptime;
        // more in /proc/self/ or /proc/<pid>
    };

    struct Power {
        // TODO @basshelal Thu 22-Jan-2026 : Can we get information about system power?
    };

    // TODO @basshelal Thu 22-Jan-2026 : Find out more things to add like:
    //  iowait, is_throttled, CAN bus health and rate and dropped packets/frames, 4g/LTE signal strength, GPS status,
    //  context switches, interrupts
};
}  // namespace pican::sysinfo
