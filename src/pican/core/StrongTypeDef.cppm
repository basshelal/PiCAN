module;
#include <utility>
export module pican.core:StrongTypeDef;

export namespace pican {
template<typename TP, typename Tag_TP>
class StrongTypeDef {
private:
    TP value_f;

public:  // constructors
    StrongTypeDef() = default;

    explicit constexpr StrongTypeDef(const TP& value) : value_f{value} {
    }

    explicit constexpr StrongTypeDef(TP&& value) : value_f{std::move(value)} {
    }

public:  // copy-control
    StrongTypeDef(const StrongTypeDef& rhs) = default;

    StrongTypeDef(StrongTypeDef&& rhs) = default;

    StrongTypeDef&
    operator=(const StrongTypeDef& rhs) = default;

    StrongTypeDef&
    operator=(StrongTypeDef&& rhs) = default;

    ~StrongTypeDef() = default;

public:  // getters
    [[nodiscard]]
    const TP&
    get() const {
        return this->value_f;
    }

    [[nodiscard]]
    TP&
    get() {
        return this->value_f;
    }

    /**
     * Force all conversions to be explicit
     */
    [[nodiscard]]
    explicit
    operator TP() const {
        return this->value_f;
    }

    [[nodiscard]]
    bool
    operator<=>(const StrongTypeDef&) const = default;
};
}  // namespace pican
