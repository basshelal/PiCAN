module;

#include <atomic>

export module pican.core:MoveableAtomic;

export namespace pican {
template<typename TP>
class MoveableAtomic {
private:  // fields
    std::atomic<TP> value_f;

public:  // constructors
    explicit MoveableAtomic(const TP& value) : value_f{value} {
    }

public:  // lifetime
    MoveableAtomic(const MoveableAtomic& rhs) = delete;

    MoveableAtomic(MoveableAtomic&& rhs) noexcept : MoveableAtomic{rhs.value_f} {
    }

    MoveableAtomic&
    operator=(const MoveableAtomic& rhs) & = delete;

    MoveableAtomic&
    operator=(MoveableAtomic&& rhs) & noexcept {
        if (std::addressof(rhs) == this) {
            return *this;
        }
        this->value_f.store(rhs.value_f.load());
        return *this;
    }

    ~MoveableAtomic() = default;

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

    template<typename Self>
    [[nodiscard]]
    std::atomic<TP>&&
    atomic(this Self&& self) {
        return std::forward<Self>(self).value_f;
    }
};
}  // namespace pican
