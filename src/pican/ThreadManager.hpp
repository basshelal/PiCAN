#pragma once

#include "pican/Thread.hpp"

namespace pican {
class ThreadManager {
private:  // fields
public:   // static functions
    static void
    initialize_all();

    static void
    start_all();
};

// TODO @basshelal Tue 03-Feb-2026 : Current (calling) thread name/id/object??

}  // namespace pican
