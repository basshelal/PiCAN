#include "pican/Utils.hpp"

#include <chrono>
#include <pthread.h>

namespace pican {

Milliseconds
get_current_millis() {
    using namespace std;
    const chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
    const chrono::milliseconds sinceEpoch = chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    return sinceEpoch.count();
}
}  // namespace pican
