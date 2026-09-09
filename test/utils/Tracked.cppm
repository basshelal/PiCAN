module;

#include <atomic>
#include <memory>
#include <sstream>

#include <magic_enum/magic_enum.hpp>

#include "test/HelperMacros.hpp"

export module pican.test_utils:Tracked;

export namespace pican::test_utils {
enum class LifetimeOperation : uint8_t {
    NONE,
    CONSTRUCTOR,
    COPY_CONSTRUCTOR,
    MOVE_CONSTRUCTOR,
    COPY_ASSIGNMENT,
    MOVE_ASSIGNMENT,
};

template<typename TP>
class Tracked {
public:  // types
    // dangerous because may call new if large enough but because this is only for testing purposes it's not yet
    // worth re-implementing
    using LifetimeCallback = std::function<void(const Tracked<TP>&)>;

public:  // constants
    static constexpr auto DEFAULT_LIFETIME_CALLBACK = [](const Tracked<TP>&) -> void {
    };

    static_assert(std::is_convertible_v<decltype(DEFAULT_LIFETIME_CALLBACK), LifetimeCallback>);

public:  // types
    struct LifetimeCallbacks {
        LifetimeCallback onConstructed = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onCopyConstructed = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onCopyConstructedFrom = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onMoveConstructed = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onMoveConstructedFrom = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onCopyAssigned = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onCopyAssignedFrom = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onMoveAssigned = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onMoveAssignedFrom = DEFAULT_LIFETIME_CALLBACK;
        LifetimeCallback onDestructed = DEFAULT_LIFETIME_CALLBACK;
    };

public:  // member fields
    TP data;
    std::atomic_int32_t copyCount;
    std::atomic_int32_t moveCount;
    LifetimeOperation lastOperation = LifetimeOperation::NONE;
    LifetimeCallbacks callbacks;

public:  // constructors
    explicit Tracked(TP data = {}, const LifetimeCallbacks& callbacks = LifetimeCallbacks{}) :
        data{std::move(data)}, copyCount{0}, moveCount{0}, lastOperation{LifetimeOperation::CONSTRUCTOR},
        callbacks{callbacks} {
        this->callbacks.onConstructed(*this);
    }

public:  // copy-control
    Tracked(const Tracked& rhs) :
        data{rhs.data}, copyCount{1}, moveCount{0}, lastOperation{LifetimeOperation::COPY_CONSTRUCTOR},
        callbacks{rhs.callbacks} {
        rhs.callbacks.onCopyConstructedFrom(rhs);
        this->callbacks.onCopyConstructed(*this);
    }

    Tracked(Tracked&& rhs) noexcept :
        data{std::move(rhs.data)}, copyCount{0}, moveCount{1}, lastOperation{LifetimeOperation::MOVE_CONSTRUCTOR},
        callbacks{rhs.callbacks}  // don't do std::move so that we can use destructor callback
    {
        rhs.callbacks.onMoveConstructedFrom(rhs);
        this->callbacks.onMoveConstructed(*this);
    }

    Tracked&
    operator=(const Tracked& rhs) & {
        if (&rhs == this) {
            return *this;
        }
        rhs.callbacks.onCopyAssignedFrom(rhs);
        this->data = rhs.data;
        this->copyCount++;
        this->lastOperation = LifetimeOperation::COPY_ASSIGNMENT;
        this->callbacks = rhs.callbacks;
        this->callbacks.onCopyAssigned(*this);
        return *this;
    }

    Tracked&
    operator=(Tracked&& rhs) & noexcept {
        rhs.callbacks.onMoveAssignedFrom(rhs);
        this->data = std::move(rhs.data);
        this->moveCount++;
        this->lastOperation = LifetimeOperation::MOVE_ASSIGNMENT;
        this->callbacks = rhs.callbacks;  // don't do std::move so that we can use destructor callback
        this->callbacks.onMoveAssigned(*this);
        return *this;
    }

    ~Tracked() {
        this->callbacks.onDestructed(*this);  // safe because callbacks don't get std::move'd
    }

public:  // member functions
    const Tracked*
    address() const {
        return this;
    }

    bool
    operator==(const Tracked& rhs) const {
        return this->data == rhs.data && this->copyCount == rhs.copyCount && this->moveCount == rhs.moveCount &&
               this->lastOperation == rhs.lastOperation;
    }

public:  // friends
    friend std::string
    to_string(const Tracked& tracked) {
        std::stringstream stream;
        stream << "Tracked { " << "data = \"" << tracked.data << "\", copyCount = " << tracked.copyCount
               << ", moveCount = " << tracked.moveCount
               << ", lastOperation = " << magic_enum::enum_name(tracked.lastOperation) << " }";
        return stream.str();
    }

    friend std::ostream&
    operator<<(std::ostream& stream, const Tracked& tracked) {
        return stream << to_string(tracked);
    }
};
}  // namespace pican::test_utils
