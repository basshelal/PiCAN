#include "core/EventNotifier.hpp"
#include "core/RingBuffer.hpp"
#include "core/Types.hpp"
#include "core/can/ReaderThread.hpp"
#include "qtui/Application.hpp"

static core::EventNotifier eventNotifier{};

static core::RingBuffer<core::can::Frame> ringBuffer{
    65'536,
    core::RingBufferOverflowBehavior::DEFAULT,
    [](const core::RingBuffer<core::can::Frame>&, const core::can::Frame& frame) -> void {
        fmt::println(fmt::runtime("Overflow! {}"), to_string(frame));
    },
    [](const core::RingBuffer<core::can::Frame>&) -> void {
        fmt::println(stderr, "Underflow!");
    },
};

static void
consumerThreadRunnable() {
    while (eventNotifier.is_active()) {
        eventNotifier.wait_blocking();
        core::can::Frame* maybeFrame = ringBuffer.pop();
        if (maybeFrame != nullptr) {
            fmt::println(to_string(*maybeFrame));
        }
    }
}

static void
producerCallback(const core::can::Frame& frame) {
    ringBuffer.push_copy(frame);
    eventNotifier.notify();
}

int
main(int argc, char** argv) {
    std::thread canFramesConsumer{consumerThreadRunnable};

    core::can::ReaderThread readerThread{"vcan0", producerCallback};

    readerThread.start();

    sleep(10);

    eventNotifier.close();

    sleep(10);
}
