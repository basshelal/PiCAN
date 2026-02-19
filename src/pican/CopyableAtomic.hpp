#pragma once

#include <atomic>

namespace pican {
template<typename TP>
class CopyableAtomic {
private:  // fields
    std::atomic<TP> value_f;

public:  // constructors
    explicit CopyableAtomic(const TP& value) : value_f{value} {
    }

public:  // lifetime
    CopyableAtomic(const CopyableAtomic& rhs) : CopyableAtomic{rhs.value_f} {
    }

    CopyableAtomic(CopyableAtomic&& rhs) noexcept : CopyableAtomic{rhs.value_f} {
    }

    CopyableAtomic&
    operator=(const CopyableAtomic& rhs) & {
        if (std::addressof(rhs) == this) {
            return *this;
        }
        this->value_f.store(rhs.value_f.load());
        return *this;
    }

    CopyableAtomic&
    operator=(CopyableAtomic&& rhs) & noexcept {
        if (std::addressof(rhs) == this) {
            return *this;
        }
        this->value_f.store(rhs.value_f.load());
        return *this;
    }

    ~CopyableAtomic() = default;

public:  // member functions
    [[nodiscard]]
    TP
    load(std::memory_order order = std::memory_order_seq_cst) const& {
        return this->value_f.load(order);
    }

    void
    store(TP value, std::memory_order order = std::memory_order_seq_cst) & {
        this->value_f.store(value, order);
    }

    [[nodiscard]]
    const std::atomic<TP>&
    atomic() const& {
        return this->value_f;
    }

    [[nodiscard]]
    std::atomic<TP>&
    atomic() & {
        return this->value_f;
    }
};
}  // namespace pican
