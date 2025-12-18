#pragma once

#include <functional>
#include <memory>

#include "Utils.hpp"
#include "core/Types.hpp"

namespace core {
enum class RingBufferOverflowBehavior : UInt8 {
    OVERWRITE_OLDEST,
    DISCARD_NEWEST,
    DEFAULT = OVERWRITE_OLDEST,
};

template<typename TP>
class RingBuffer {
public:  // types
    using UnderflowCallback = std::function<void(const RingBuffer<TP>&)>;
    using OverflowCallback = std::function<void(const RingBuffer<TP>&, const TP&)>;

public:  // static constants
    static constexpr auto DEFAULT_UNDERFLOW_CALLBACK = [](const RingBuffer<TP>&) -> void {
    };

    static constexpr auto DEFAULT_OVERFLOW_CALLBACK = [](const RingBuffer<TP>&, const TP&) -> void {
    };

private:  // member fields
    Size capacity_f;
    RingBufferOverflowBehavior overflowBehavior_f;
    OverflowCallback overflowCallback_f;
    UnderflowCallback underflowCallback_f;
    Size writeIndex_f;
    Size readIndex_f;
    std::unique_ptr<TP[]> data_f;

    // TODO: Sunday, December 14, 2025 @basshelal: A constructor where the memory is
    //  pre-allocated, deleter is customized too, probably just take a unique_ptr

    // TODO: Sunday, December 14, 2025 @basshelal: Implement specific overflow behaviors

    // TODO: Sunday, December 14, 2025 @basshelal: To save on memory, we can allow for
    //  a filter function as well
public:  // constructors
    explicit RingBuffer(
        Size capacity, RingBufferOverflowBehavior overflowBehavior, OverflowCallback overflowCallback,
        UnderflowCallback underflowCallback
    ) :
        capacity_f(capacity + 1), overflowBehavior_f(overflowBehavior), overflowCallback_f(std::move(overflowCallback)),
        underflowCallback_f(std::move(underflowCallback)), writeIndex_f(0), readIndex_f(0),
        data_f(new TP[this->capacity_f]) {
    }

    explicit RingBuffer(Size capacity) :
        RingBuffer(
            capacity, RingBufferOverflowBehavior::DEFAULT, RingBuffer::DEFAULT_OVERFLOW_CALLBACK,
            RingBuffer::DEFAULT_UNDERFLOW_CALLBACK
        ) {
    }

    // TODO: Sunday, December 14, 2025 @basshelal: Remove copying behavior, should be move only!
public:  // copy-control
    RingBuffer(const RingBuffer& rhs) :
        capacity_f(rhs.capacity_f), overflowBehavior_f(rhs.overflowBehavior_f),
        overflowCallback_f(rhs.overflowCallback_f), underflowCallback_f(rhs.underflowCallback_f),
        writeIndex_f(rhs.writeIndex_f), readIndex_f(rhs.readIndex_f), data_f(new TP[rhs.capacity_f]) {
        for (Size i = 0; i < rhs.capacity_f; i++) {
            this->data_f[i] = rhs.data_f[i];
        }
    }

    RingBuffer(RingBuffer&& rhs) noexcept = default;

    RingBuffer&
    operator=(const RingBuffer& rhs) & {
        this->capacity_f = rhs.capacity_f;
        this->data_f.reset(new TP[rhs.capacity_f]);
        for (Size i = 0; i < rhs.capacity_f; i++) {
            this->data_f[i] = rhs.data_f[i];
        }
        this->overflowCallback_f = rhs.overflowCallback_f;
        this->underflowCallback_f = rhs.underflowCallback_f;
        this->overflowBehavior_f = rhs.overflowBehavior_f;
        this->writeIndex_f = rhs.writeIndex_f;
        this->readIndex_f = rhs.readIndex_f;
        return *this;
    }

    RingBuffer&
    operator=(RingBuffer&& rhs) & = default;

    ~RingBuffer() = default;

public:  // getters
    [[nodiscard]]
    inline Size
    capacity() const {
        return this->capacity_f - 1;
    }

public:  // member functions
    [[nodiscard]]
    inline Size
    size() const {
        if (this->readIndex_f == this->writeIndex_f) {
            return 0;
        } else if (this->writeIndex_f > this->readIndex_f) {
            return this->writeIndex_f - this->readIndex_f;
        } else {
            assert(this->readIndex_f > this->writeIndex_f);
            const Size diff = this->capacity_f - readIndex_f;
            return diff + this->writeIndex_f;
        }
    }

    [[nodiscard]]
    inline bool
    is_empty() const {
        return this->size() == 0;
    }

    [[nodiscard]]
    inline bool
    is_full() const {
        return this->size() == this->capacity();
    }

    [[nodiscard]]
    inline TP*
    peek() const {
        if (this->is_empty()) {
            return nullptr;
        }
        return this->data_f.get() + this->readIndex_f;
    }

    [[nodiscard]]
    inline TP*
    pop() {
        if (this->readIndex_f == this->writeIndex_f) {
            this->underflowCallback_f(*this);
            return nullptr;
        }
        if (this->readIndex_f == this->capacity_f) {
            this->readIndex_f = 0;
        }
        TP* val = this->data_f.get() + this->readIndex_f;
        ++this->readIndex_f;
        return val;
    }

    inline void
    push_copy(const TP& val) {
        if (this->index_incremented(this->writeIndex_f) == this->readIndex_f) {
            this->handle_overflow(val);
            return;
        }
        if (this->writeIndex_f == this->capacity_f) {
            this->writeIndex_f = 0;
        }
        this->data_f[this->writeIndex_f] = val;
        ++this->writeIndex_f;
    }

    template<typename Iterable_TP>
    inline void
    push_copy_all(const Iterable_TP& iterable) {
        for (const TP& element : iterable) {
            this->push_copy(element);
        }
    }

    inline void
    push_move(TP&& val) {
        if (this->index_incremented(this->writeIndex_f) == this->readIndex_f) {
            this->handle_overflow(val);
            return;
        }
        if (this->writeIndex_f == this->capacity_f) {
            this->writeIndex_f = 0;
        }
        this->data_f[this->writeIndex_f] = std::move(val);
        ++this->writeIndex_f;
    }

    template<typename Iterable_TP>
    inline void
    push_move_all(Iterable_TP&& iterable) {
        for (TP& element : iterable) {
            this->push_move(std::move(element));
        }
    }

    inline void
    clear() {
        this->writeIndex_f = 0;
        this->readIndex_f = 0;
    }

private:  // methods
    [[nodiscard]]
    inline Size
    index_incremented(Size index) const {
        if ((index + 1) == this->capacity_f) {
            return 0;
        }
        return index + 1;
    }

    inline void
    handle_overflow(const TP& element) const {
        switch (this->overflowBehavior_f) {
            case RingBufferOverflowBehavior::OVERWRITE_OLDEST: {
                // TODO: Sunday, December 14, 2025 @basshelal: Write to end?
                break;
            }
            case RingBufferOverflowBehavior::DISCARD_NEWEST: {
                break;
            }
        }
        if (this->overflowCallback_f != nullptr) {
            this->overflowCallback_f(*this, element);
        }
    }

    inline void
    handle_underflow() const {
        if (this->underflowCallback_f != nullptr) {
            this->underflowCallback_f(*this);
        }
    }
};
}  // namespace core
