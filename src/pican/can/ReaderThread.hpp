#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include <linux/can.h>
#include <sys/socket.h>

#include "pican/can/Frame.hpp"

namespace pican::can {
class ReaderThread {
public:  // types
    using Callback = std::function<void(const pican::can::Frame&)>;

private:  // fields
    std::string interfaceName_f;
    int socketFd_f = -1;
    std::atomic_bool isRunning_f{false};
    std::thread thread_f;
    Callback callback_f;

public:  // constructors
    explicit ReaderThread(const std::string& interfaceName, const ReaderThread::Callback& callback) :
        interfaceName_f{interfaceName}, socketFd_f{-1}, isRunning_f{false}, thread_f{}, callback_f{callback} {
    }

public:  // copy control
    ~ReaderThread() {
        this->stop();
    }

public:  // member functions
    void
    start();

    [[nodiscard]]
    inline bool
    is_running() const {
        return this->isRunning_f.load();
    }

    void
    stop();

private:  // helper functions
    void
    read_loop();
};
}  // namespace pican::can
