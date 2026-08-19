module;

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>

#include "pican/contracts.hpp"

export module pican.ds:RingBuffer;

import :Array;
import pican.core;
import pican.mem;

export namespace pican::ds {

enum class RingBufferOverflowBehavior : std::uint8_t {
    OVERWRITE_OLDEST = 0,
    DISCARD_NEWEST = 1,
    DEFAULT = OVERWRITE_OLDEST,
};

template<typename TP>
class RingBuffer {
public:  // types
    // TODO @basshelal Sun 26-Jul-2026 : Remove all functions and use bare C style functions
    // using OverflowCallback = std::function<void(const RingBuffer<TP>&)>;
    using Callback = void (*)(const RingBuffer<TP>&);
    using OverflowCallback = Callback;
    using UnderflowCallback = Callback;
    using OverwriteCallback = Callback;
    using FailedReadCallback = Callback;

    static constexpr auto DEFAULT_CALLBACK = [](const RingBuffer<TP>&) -> void {
    };

private:  // member fields
    Array<TP> array_f;
    Count itemsCount_f;
    RingBufferOverflowBehavior overflowBehavior_f;
    OverflowCallback overflowCallback_f;
    UnderflowCallback underflowCallback_f;
    OverwriteCallback overwriteCallback_f;
    FailedReadCallback failedReadCallback_f;
    alignas(pican::mem::CACHE_LINE_ALIGNMENT) pican::CopyableAtomic<Index> writeIndex_f;
    alignas(pican::mem::CACHE_LINE_ALIGNMENT) pican::CopyableAtomic<Index> readIndex_f;

    // both above need to be on separate cache lines in order to avoid false-sharing
    // this is because each one will be used by its own thread, thus typically by a separate core,
    // if they are on the _same_ cache line then any modification to either by one core will need a
    // re-fetch by the other core because it is deemed as "dirty", therefore, we need to force them
    // to be on different cache lines to avoid this issue
    // this matters for performance of hotspots and hot loops

public:  // constructors
    RingBuffer(
        const Array<TP>& array, RingBufferOverflowBehavior overflowBehavior, OverflowCallback overflowCallback,
        UnderflowCallback underflowCallback, OverwriteCallback overwriteCallback, FailedReadCallback failedReadCallback
    ) :
        array_f{array}, itemsCount_f{array.length() - 1}, overflowBehavior_f{overflowBehavior},
        overflowCallback_f{overflowCallback}, underflowCallback_f{underflowCallback},
        overwriteCallback_f{overwriteCallback}, failedReadCallback_f{failedReadCallback}, writeIndex_f{0},
        readIndex_f{0} {
    }

    RingBuffer(const Array<TP>& array, RingBufferOverflowBehavior overflowBehavior) :
        RingBuffer{array, overflowBehavior, DEFAULT_CALLBACK, DEFAULT_CALLBACK, DEFAULT_CALLBACK, DEFAULT_CALLBACK} {
    }


public:  // copy-control
    RingBuffer(const RingBuffer& rhs) = delete;

    RingBuffer(RingBuffer&& rhs) noexcept = default;

    RingBuffer&
    operator=(const RingBuffer& rhs) & = delete;

    RingBuffer&
    operator=(RingBuffer&& rhs) & noexcept = default;

    ~RingBuffer() = default;

public:  // getters
    [[nodiscard]]
    Count
    capacity() const& {
        return this->itemsCount_f;
    }

    [[nodiscard]]
    Count
    size() const& {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_acquire);
        const Index readIndex = this->readIndex_f.load(std::memory_order_acquire);

        if (readIndex == writeIndex) {  // empty
            return 0;
        }

        if (writeIndex > readIndex) {  // no circling has happened yet
            return writeIndex - readIndex;
        }
        CONTRACTS_ASSERT(readIndex > writeIndex);

        // here, a circling has happened
        const Count diff = this->capacity() - readIndex;
        return diff + writeIndex;
    }

    [[nodiscard]]
    bool
    is_empty() const& {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_acquire);
        const Index readIndex = this->readIndex_f.load(std::memory_order_acquire);

        return readIndex == writeIndex;
    }

    [[nodiscard]]
    bool
    is_full() const& {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_acquire);
        const Index readIndex = this->readIndex_f.load(std::memory_order_acquire);

        return this->increment_index(writeIndex) == readIndex;
    }

    [[nodiscard]]
    RingBufferOverflowBehavior
    overflow_behavior() const& {
        return this->overflowBehavior_f;
    }

public:  // member functions
    void
    push_copy(const TP& val) & {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_acquire);

        if (this->is_full()) {
            if (this->overflowBehavior_f == RingBufferOverflowBehavior::DISCARD_NEWEST) {
                this->overflowCallback_f(*this);
                return;
            }
        }

        this->array_f.set_copy(writeIndex, val);  // uses copy assignment

        if (this->overflowBehavior_f == RingBufferOverflowBehavior::OVERWRITE_OLDEST) {
            this->overwriteCallback_f(*this);
        }

        // increment it correctly
        if (writeIndex == this->capacity()) {
            this->writeIndex_f.store(0, std::memory_order_release);
        } else {
            this->writeIndex_f.store(writeIndex + 1, std::memory_order_release);
        }
    }

    [[nodiscard]]
    TP*
    pop_ptr() & {
        Index writeIndex = this->writeIndex_f.load(std::memory_order_acquire);
        const Index readIndex = this->readIndex_f.load(std::memory_order_acquire);

        if (readIndex == writeIndex) {  // empty!
            this->underflowCallback_f(*this);
            return nullptr;
        }

        bool writeHappened = false;
        TP* popped = nullptr;
        do {
            popped = this->array_f.get_ptr(readIndex);

            const Index latestWriteIndex = this->writeIndex_f.load(std::memory_order_acquire);
            writeHappened = latestWriteIndex != writeIndex;
            if (writeHappened) {
                this->failedReadCallback_f(*this);
                writeIndex = latestWriteIndex;
            }
        } while (writeHappened);

        if (readIndex == this->capacity()) {
            this->readIndex_f.store(0, std::memory_order_release);
        } else {
            this->readIndex_f.store(readIndex + 1, std::memory_order_release);
        }

        return popped;
    }

    [[nodiscard]]
    std::optional<TP>
    pop_copy() & {
        TP* ptr = this->pop_ptr();
        if (ptr == nullptr) {
            return std::optional<TP>{nullptr};
        }

        return std::optional<TP>{*ptr};
    }

    [[nodiscard]]
    std::optional<TP>
    pop_move() & {
        TP* ptr = this->pop_ptr();
        if (ptr == nullptr) {
            return std::optional<TP>{};
        }

        return std::optional<TP>{std::move(*ptr)};
    }

    void
    clear() & {
        this->writeIndex_f.store(0, std::memory_order_release);
        this->readIndex_f.store(0, std::memory_order_release);
    }

private:  // member functions
    void
    increment_index_in_place(std::atomic<Index>& index) & {
        if (index.load(std::memory_order_acquire) == this->capacity()) {
            index.store(0, std::memory_order_release);
            return;
        }
        index.fetch_add(1);
    }

    Index
    increment_index(Index index) const& {
        if (index == this->capacity()) {  // circular behavior
            return 0;
        }
        return index + 1;
    }
};
}  // namespace pican::ds
