#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "pican/Array.hpp"
#include "pican/Contracts.hpp"
#include "pican/Result.hpp"
#include "pican/Types.hpp"
#include "pican/Utils.hpp"
#include "pican/mem/Manager.hpp"
#include "pican/mem/Utils.hpp"

namespace pican {

enum class RingBufferOverflowBehavior : std::uint8_t {
    OVERWRITE_OLDEST = 0,
    DISCARD_NEWEST = 1,
    DEFAULT = OVERWRITE_OLDEST,
};

template<typename TP>
class RingBuffer {
public:  // types
    using OverflowCallback = std::function<void(const RingBuffer<TP>&)>;
    using UnderflowCallback = std::function<void(const RingBuffer<TP>&)>;
    using OverwriteCallback = std::function<void(const RingBuffer<TP>&)>;
    using FailedReadCallback = std::function<void(const RingBuffer<TP>&)>;

    static constexpr auto DEFAULT_CALLBACK = [](const RingBuffer<TP>&) -> void {
    };

private:  // member fields
    pican::Array<TP> array_f;
    Count itemsCount_f;
    RingBufferOverflowBehavior overflowBehavior_f;
    OverflowCallback overflowCallback_f;
    UnderflowCallback underflowCallback_f;
    OverwriteCallback overwriteCallback_f;
    FailedReadCallback failedReadCallback_f;

    // both below need to be on separate cache lines in order to avoid false-sharing
    // this is because each one will be used by its own thread, thus typically by a separate core,
    // if they are on the _same_ cache line then any modification to either by one core will need a
    // re-fetch by the other core because it is deemed as "dirty", therefore, we need to force them
    // to be on different cache lines to avoid this issue
    // this matters for performance of hotspots and hot loops

    alignas(pican::mem::CACHE_LINE_ALIGNMENT) std::atomic<Index> writeIndex_f;

    alignas(pican::mem::CACHE_LINE_ALIGNMENT) std::atomic<Index> readIndex_f;

    alignas(pican::mem::CACHE_LINE_ALIGNMENT) std::atomic<Count> itemsWritten_f;

public:  // constructors
    RingBuffer(
        const pican::Array<TP>& array, RingBufferOverflowBehavior overflowBehavior, OverflowCallback overflowCallback,
        UnderflowCallback underflowCallback, OverwriteCallback overwriteCallback, FailedReadCallback failedReadCallback
    ) :
        array_f{array}, itemsCount_f{array.items_count() - 1}, overflowBehavior_f{overflowBehavior},
        overflowCallback_f{overflowCallback}, underflowCallback_f{underflowCallback},
        overwriteCallback_f{overwriteCallback}, failedReadCallback_f{failedReadCallback}, writeIndex_f{0},
        readIndex_f{0}, itemsWritten_f{0} {
    }

    RingBuffer(const pican::Array<TP>& array, RingBufferOverflowBehavior overflowBehavior) :
        RingBuffer{array, overflowBehavior, DEFAULT_CALLBACK, DEFAULT_CALLBACK, DEFAULT_CALLBACK, DEFAULT_CALLBACK} {
    }


public:  // copy-control
    RingBuffer(const RingBuffer& rhs) = delete;

    RingBuffer(RingBuffer&& rhs) noexcept = delete;

    RingBuffer&
    operator=(const RingBuffer& rhs) & = delete;

    RingBuffer&
    operator=(RingBuffer&& rhs) & noexcept = delete;

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
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_relaxed);
        const Index readIndex = this->readIndex_f.load(std::memory_order_relaxed);

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
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_relaxed);
        const Index readIndex = this->readIndex_f.load(std::memory_order_relaxed);

        return readIndex == writeIndex;
    }

    [[nodiscard]]
    bool
    is_full() const& {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_relaxed);
        const Index readIndex = this->readIndex_f.load(std::memory_order_relaxed);

        return this->increment_index(writeIndex) == readIndex;
    }

    [[nodiscard]]
    RingBufferOverflowBehavior
    overflow_behavior() const& {
        return this->overflowBehavior_f;
    }

public:  // member functions
    // TODO fix memory orders by researching them exactly

    void
    push_copy(const TP& val) & {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_relaxed);
        [[maybe_unused]]
        const Index readIndex = this->readIndex_f.load(std::memory_order_relaxed);

        if (this->is_full()) {
            if (this->overflowBehavior_f == RingBufferOverflowBehavior::DISCARD_NEWEST) {
                this->overflowCallback_f(*this);
                return;
            }
        }

        this->array_f.set_at(writeIndex, val);  // uses copy assignment

        if (this->overflowBehavior_f == RingBufferOverflowBehavior::OVERWRITE_OLDEST) {
            this->overwriteCallback_f(*this);
        }

        // increment it correctly
        if (writeIndex == this->capacity()) {
            this->writeIndex_f.store(0, std::memory_order_relaxed);
        } else {
            this->writeIndex_f.store(writeIndex + 1, std::memory_order_relaxed);
        }
    }

    [[nodiscard]]
    TP*
    pop_ptr() & {
        const Index writeIndex = this->writeIndex_f.load(std::memory_order_relaxed);
        const Index readIndex = this->readIndex_f.load(std::memory_order_relaxed);

        if (readIndex == writeIndex) {  // empty!
            this->underflowCallback_f(*this);
            return nullptr;
        }

        bool writeHappened = false;
        TP* popped = nullptr;
        do {
            popped = this->array_f.get_at_ptr(readIndex);

            const Index latestWriteIndex = this->writeIndex_f.load(std::memory_order_relaxed);
            writeHappened = latestWriteIndex != writeIndex;
            if (writeHappened) {
                this->failedReadCallback_f(*this);
            }
        } while (writeHappened);

        if (readIndex == this->capacity()) {
            this->readIndex_f.store(0, std::memory_order_relaxed);
        } else {
            this->readIndex_f.store(readIndex + 1, std::memory_order_relaxed);
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
        this->writeIndex_f.store(0, std::memory_order_relaxed);
        this->readIndex_f.store(0, std::memory_order_relaxed);
    }

private:  // member functions
    void
    increment_index_in_place(std::atomic<Index>& index) & {
        if (index.load(std::memory_order_relaxed) == this->capacity()) {
            index.store(0, std::memory_order_relaxed);
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
}  // namespace pican
