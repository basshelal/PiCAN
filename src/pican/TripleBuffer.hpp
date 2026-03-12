#pragma once

#include <array>
#include <cstdint>

#include "pican/CopyableAtomic.hpp"
#include "pican/Types.hpp"

namespace pican {

template<typename TP>
class TripleBuffer {
    // static_assert(std::is_trivially_copyable_v<TP>, "Type must be trivially copyable");
    static_assert(std::is_copy_assignable_v<TP>, "Type must be copy assignable");

private:  // types
    struct State {
        bool hasNewData;
        std::uint8_t newDataIndex;
        std::uint16_t _ignore;
    };

    static_assert(sizeof(State) == 4, "State must be exactly 4 bytes");
    static_assert(std::atomic<State>::is_always_lock_free, "Atomic operations must be lock-free");

private:  // fields
    std::array<TP, 3> array_f;
    CopyableAtomic<State> state_f;
    std::uint8_t writeIndex_f;
    std::uint8_t readIndex_f;

public:  // constructors
    TripleBuffer(const TP& initial) :
        array_f{initial, initial, initial}, state_f{State{false, 0, 0}}, writeIndex_f{1}, readIndex_f{2} {
    }

    TripleBuffer() : TripleBuffer{TP{}} {
    }

public:  // lifetime
    TripleBuffer(const TripleBuffer& rhs) = delete;

    TripleBuffer(TripleBuffer&& rhs) noexcept = default;

    TripleBuffer&
    operator=(const TripleBuffer& rhs) & = delete;

    TripleBuffer&
    operator=(TripleBuffer&& rhs) & noexcept = default;

    ~TripleBuffer() = default;

public:  // member functions
    void
    write(const TP& value) & {
        this->array_f[this->writeIndex_f] = value;
        State currentState = this->state_f.load(std::memory_order_acquire);
        State nextState{};

        do {
            nextState.hasNewData = true;
            nextState.newDataIndex = this->writeIndex_f;
        } while (!this->state_f.atomic().compare_exchange_weak(
            currentState, nextState, std::memory_order_acq_rel, std::memory_order_acquire
        ));
        // above means:
        // if (this->state_f.load(std::memory_order_acquire) == currentState) {
        //     this->state_f.store(nextState, std::memory_order_release);
        //     break;
        // } else {
        //     currentState = this->state_f.load(std::memory_order_relaxed);
        // }

        this->writeIndex_f = currentState.newDataIndex;
    }

    bool
    has_new() const& {
        State current = this->state_f.load(std::memory_order_acquire);
        return current.hasNewData;
    }

    [[nodiscard]]
    TP
    read() & {
        State currentState = this->state_f.load(std::memory_order_acquire);

        if (!currentState.hasNewData) {
            return this->array_f[this->readIndex_f];
        }

        State nextState{};
        do {
            nextState.newDataIndex = this->readIndex_f;
            nextState.hasNewData = false;

        } while (!this->state_f.atomic().compare_exchange_weak(
            currentState, nextState, std::memory_order_acq_rel, std::memory_order_acquire
        ));
        // above means:
        // if (this->state_f.load(std::memory_order_acquire) == currentState) {
        //     this->state_f.store(nextState, std::memory_order_release);
        //     break;
        // } else {
        //     currentState = this->state_f.load(std::memory_order_relaxed);
        // }

        this->readIndex_f = currentState.newDataIndex;
        return this->array_f[this->readIndex_f];
    }
};
}  // namespace pican
