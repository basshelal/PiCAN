export module pican.core:ScopeGuard;

export namespace pican::core {

template<typename Callable_TP>
class ScopeGuard {
private:
    Callable_TP callable_f;

public:
    [[nodiscard]]
    ScopeGuard(const Callable_TP& callable) : callable_f{callable} {  // NOLINT(*-explicit-conversions)
    }

    ~ScopeGuard() {
        if (callable_f != nullptr) {
            callable_f();
        }
    }

public:  // Lifetime
    ScopeGuard(const ScopeGuard& rhs) = default;

    ScopeGuard(ScopeGuard&& rhs) noexcept = default;

    ScopeGuard&
    operator=(const ScopeGuard& rhs) = default;

    ScopeGuard&
    operator=(ScopeGuard&& rhs) noexcept = default;
};

}  // namespace pican::core
